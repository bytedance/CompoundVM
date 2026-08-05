/*
 * Copyright (c) 1997, 2017, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

package java.lang.ref;

import sun.misc.Cleaner;
import sun.misc.JavaLangRefAccess;
import sun.misc.SharedSecrets;
import jdk.internal.vm.annotation.IntrinsicCandidate;
import jdk.internal.vm.annotation.ForceInline;

/**
 * Abstract base class for reference objects.  This class defines the
 * operations common to all reference objects.  Because reference objects are
 * implemented in close cooperation with the garbage collector, this class may
 * not be subclassed directly.
 *
 * @author   Mark Reinhold
 * @since    1.2
 */

public abstract class Reference<T> {

    /* A Reference instance is in one of four possible internal states:
     *
     *     Active: Subject to special treatment by the garbage collector.  Some
     *     time after the collector detects that the reachability of the
     *     referent has changed to the appropriate state, it changes the
     *     instance's state to either Pending or Inactive, depending upon
     *     whether or not the instance was registered with a queue when it was
     *     created.  In the former case it also adds the instance to the
     *     pending-Reference list.  Newly-created instances are Active.
     *
     *     Pending: An element of the pending-Reference list, waiting to be
     *     enqueued by the Reference-handler thread.  Unregistered instances
     *     are never in this state.
     *
     *     Enqueued: An element of the queue with which the instance was
     *     registered when it was created.  When an instance is removed from
     *     its ReferenceQueue, it is made Inactive.  Unregistered instances are
     *     never in this state.
     *
     *     Inactive: Nothing more to do.  Once an instance becomes Inactive its
     *     state will never change again.
     *
     * The state is encoded in the queue and next fields as follows:
     *
     *     Active: queue = ReferenceQueue with which instance is registered, or
     *     ReferenceQueue.NULL if it was not registered with a queue; next =
     *     null.
     *
     *     Pending: queue = ReferenceQueue with which instance is registered;
     *     next = this
     *
     *     Enqueued: queue = ReferenceQueue.ENQUEUED; next = Following instance
     *     in queue, or this if at end of list.
     *
     *     Inactive: queue = ReferenceQueue.NULL; next = this.
     *
     * With this scheme the collector need only examine the next field in order
     * to determine whether a Reference instance requires special treatment: If
     * the next field is null then the instance is active; if it is non-null,
     * then the collector should treat the instance normally.
     *
     * To ensure that a concurrent collector can discover active Reference
     * objects without interfering with application threads that may apply
     * the enqueue() method to those objects, collectors should link
     * discovered objects through the discovered field. The discovered
     * field is also used for linking Reference objects in the pending list.
     */

    private T referent;         /* Treated specially by GC */

    volatile ReferenceQueue<? super T> queue;

    /* When active:   NULL
     *     pending:   this
     *    Enqueued:   next reference in queue (or this if last)
     *    Inactive:   this
     */
    @SuppressWarnings("rawtypes")
    volatile Reference next;

    /* When active:   next element in a discovered reference list maintained by GC (or this if last)
     *     pending:   next element in the pending list (or null if last)
     *   otherwise:   NULL
     */
    transient private Reference<T> discovered;  /* used by VM */


    /* Object used to synchronize with the garbage collector.  The collector
     * must acquire this lock at the beginning of each collection cycle.  It is
     * therefore critical that any code holding this lock complete as quickly
     * as possible, allocate no new objects, and avoid calling user code.
     */
    static private class Lock { }
    private static Lock lock = new Lock();


    /* List of References waiting to be enqueued.  The collector adds
     * References to this list, while the Reference-handler thread removes
     * them.  This list is protected by the above lock object. The
     * list uses the discovered field to link its elements.
     */
    private static Reference<?> pending = null;

    /*
     * Atomically get and clear (set to null) the VM's pending-Reference list.
     */
    private static native Reference<?> getAndClearReferencePendingList();

    /*
     * Test whether the VM's pending-Reference list contains any entries.
     */
    private static native boolean hasReferencePendingList();

    /*
     * Wait until the VM's pending-Reference list may be non-null.
     */
    private static native void waitForReferencePendingList();



    /* High-priority thread to enqueue pending References
     */
    private static class ReferenceHandler extends Thread {

        private static void ensureClassInitialized(Class<?> clazz) {
            try {
                Class.forName(clazz.getName(), true, clazz.getClassLoader());
            } catch (ClassNotFoundException e) {
                throw (Error) new NoClassDefFoundError(e.getMessage()).initCause(e);
            }
        }

        static {
            // pre-load and initialize InterruptedException and Cleaner classes
            // so that we don't get into trouble later in the run loop if there's
            // memory shortage while loading/initializing them lazily.
            ensureClassInitialized(InterruptedException.class);
            ensureClassInitialized(Cleaner.class);
        }

        ReferenceHandler(ThreadGroup g, String name) {
            super(g, name);
        }

        public void run() {
            while (true) {
                tryHandlePending(true);
            }
        }
    }

    /*
     * Enqueue a Reference taken from the pending list.  Calling this method
     * takes us from the Reference<?> domain of the pending list elements to
     * having a Reference<T> with a correspondingly typed queue.
     */
    private void enqueueFromPending() {
        ReferenceQueue<? super T> q = queue;
        if (q != ReferenceQueue.NULL) q.enqueue(this);
    }

    /**
     * Try handle pending {@link Reference} if there is one.<p>
     * Return {@code true} as a hint that there might be another
     * {@link Reference} pending or {@code false} when there are no more pending
     * {@link Reference}s at the moment and the program can do some other
     * useful work instead of looping.
     *
     * @param waitForNotify if {@code true} and there was no pending
     *                      {@link Reference}, wait until notified from VM
     *                      or interrupted; if {@code false}, return immediately
     *                      when there is no pending {@link Reference}.
     * @return {@code true} if there was a {@link Reference} pending and it
     *         was processed, or we waited for notification and either got it
     *         or thread was interrupted before being notified;
     *         {@code false} otherwise.
     */
    static boolean tryHandlePending(boolean waitForNotify) {
        // Only the singleton reference processing thread calls
        // waitForReferencePendingList() and getAndClearReferencePendingList().
        // These are separate operations to avoid a race with other threads
        // that are calling waitForReferenceProcessing().
        if (waitForNotify) {
            waitForReferencePendingList();
            synchronized (lock) {
                pending = getAndClearReferencePendingList();
            }
        }

        boolean handled = false;
        while (true) {
            Reference<?> ref;
            synchronized (lock) {
                if (pending == null) {
                    return handled;
                }
                ref = pending;
                pending = ref.discovered;
                ref.discovered = null;
            }
            handled = true;
            if (ref instanceof Cleaner) {
                ((Cleaner) ref).clean();
            } else {
                ref.enqueueFromPending();
            }
        }
    }

    static {
        ThreadGroup tg = Thread.currentThread().getThreadGroup();
        for (ThreadGroup tgn = tg;
             tgn != null;
             tg = tgn, tgn = tg.getParent());
        Thread handler = new ReferenceHandler(tg, "Reference Handler");
        /* If there were a special system-only priority greater than
         * MAX_PRIORITY, it would be used here
         */
        handler.setPriority(Thread.MAX_PRIORITY);
        handler.setDaemon(true);
        handler.start();

        // provide access in SharedSecrets
        SharedSecrets.setJavaLangRefAccess(new JavaLangRefAccess() {
            @Override
            public boolean tryHandlePendingReference() {
                return tryHandlePending(false);
            }
        });
    }

    /* -- Referent accessor and setters -- */

    /**
     * Tests if the referent of this reference object is {@code obj}.
     * Using a {@code null} {@code obj} returns {@code true} if the
     * reference object has been cleared.
     *
     * @param  obj the object to compare with this reference object's referent
     * @return {@code true} if {@code obj} is the referent of this reference object
     * @since 16
     */
    public final boolean refersTo(T obj) {
        return refersToImpl(obj);
    }

    /* Implementation of refersTo(), overridden for phantom references.
     * This method exists only to avoid making refersTo0() virtual. Making
     * refersTo0() virtual has the undesirable effect of C2 often preferring
     * to call the native implementation over the intrinsic.
     */
    boolean refersToImpl(T obj) {
        return refersTo0(obj);
    }

    @IntrinsicCandidate
    private native boolean refersTo0(Object o);

    /**
     * Returns this reference object's referent.  If this reference object has
     * been cleared, either by the program or by the garbage collector, then
     * this method returns <code>null</code>.
     *
     * @return   The object to which this reference refers, or
     *           <code>null</code> if this reference object has been cleared
     */
    @IntrinsicCandidate
    public T get() {
        return this.referent;
    }

    /**
     * Clears this reference object.  Invoking this method will not cause this
     * object to be enqueued.
     *
     * <p> This method is invoked only by Java code; when the garbage collector
     * clears references it does so directly, without invoking this method.
     */
    public void clear() {
        clear0();
    }

    /* Implementation of clear(), also used by enqueue().  A simple
     * assignment of the referent field won't do for some garbage
     * collectors.
     */
    private native void clear0();

    /* -- Queue operations -- */

    /**
     * Tells whether or not this reference object has been enqueued, either by
     * the program or by the garbage collector.  If this reference object was
     * not registered with a queue when it was created, then this method will
     * always return <code>false</code>.
     *
     * @return   <code>true</code> if and only if this reference object has
     *           been enqueued
     */
    public boolean isEnqueued() {
        return (this.queue == ReferenceQueue.ENQUEUED);
    }

    /**
     * Clears this reference object and adds it to the queue with which
     * it is registered, if any.
     *
     * <p> This method is invoked only by Java code; when the garbage collector
     * enqueues references it does so directly, without invoking this method.
     *
     * @return   <code>true</code> if this reference object was successfully
     *           enqueued; <code>false</code> if it was already enqueued or if
     *           it was not registered with a queue when it was created
     */
    public boolean enqueue() {
        this.referent = null;
        return this.queue.enqueue(this);
    }


    /**
     * Throws {@link CloneNotSupportedException}. A {@code Reference} cannot be
     * meaningfully cloned. Construct a new {@code Reference} instead.
     *
     * @apiNote This method is defined in Java SE 8 Maintenance Release 4.
     *
     * @return  never returns normally
     * @throws  CloneNotSupportedException always
     *
     * @since 8
     */
    @Override
    protected Object clone() throws CloneNotSupportedException {
        throw new CloneNotSupportedException();
    }

    /* -- Constructors -- */

    Reference(T referent) {
        this(referent, null);
    }

    Reference(T referent, ReferenceQueue<? super T> queue) {
        this.referent = referent;
        this.queue = (queue == null) ? ReferenceQueue.NULL : queue;
    }

    /**
     * Ensures that the object referenced by the given reference remains
     * <a href="package-summary.html#reachability"><em>strongly reachable</em></a>,
     * regardless of any prior actions of the program that might otherwise cause
     * the object to become unreachable; thus, the referenced object is not
     * reclaimable by garbage collection at least until after the invocation of
     * this method. Invocation of this method does not itself initiate garbage
     * collection or finalization.
     *
     * <p>
     * This method establishes an ordering for
     * <a href="package-summary.html#reachability"><em>strong reachability</em></a>
     * with respect to garbage collection. It controls relations that are
     * otherwise only implicit in a program -- the reachability conditions
     * triggering garbage collection. This method is designed for use in
     * uncommon situations of premature finalization where using
     * {@code synchronized} blocks or methods, or using other synchronization
     * facilities are not possible or do not provide the desired control. This
     * method is applicable only when reclamation may have visible effects,
     * which is possible for objects with finalizers (See Section {@jls 12.6}
     * of <cite>The Java Language Specification</cite>) that
     * are implemented in ways that rely on ordering control for
     * correctness.
     *
     * @apiNote
     *          Finalization may occur whenever the virtual machine detects that no
     *          reference to an object will ever be stored in the heap: The garbage
     *          collector may reclaim an object even if the fields of that object
     *          are
     *          still in use, so long as the object has otherwise become
     *          unreachable.
     *          This may have surprising and undesirable effects in cases such as
     *          the
     *          following example in which the bookkeeping associated with a class
     *          is
     *          managed through array indices. Here, method {@code action} uses a
     *          {@code reachabilityFence} to ensure that the {@code Resource} object
     *          is
     *          not reclaimed before bookkeeping on an associated
     *          {@code ExternalResource} has been performed; in particular here, to
     *          ensure that the array slot holding the {@code ExternalResource} is
     *          not
     *          nulled out in method {@link Object#finalize}, which may otherwise
     *          run
     *          concurrently.
     *
     *          <pre> {@code
     * class Resource {
     *   private static ExternalResource[] externalResourceArray = ...
     *
     *   int myIndex;
     *   Resource(...) {
     *     myIndex = ...
     *     externalResourceArray[myIndex] = ...;
     *     ...
     *   }
     *   protected void finalize() {
     *     externalResourceArray[myIndex] = null;
     *     ...
     *   }
     *   public void action() {
     *     try {
     *       // ...
     *       int i = myIndex;
     *       Resource.update(externalResourceArray[i]);
     *     } finally {
     *       Reference.reachabilityFence(this);
     *     }
     *   }
     *   private static void update(ExternalResource ext) {
     *     ext.status = ...;
     *   }
     * }}</pre>
     *
     *          Here, the invocation of {@code reachabilityFence} is nonintuitively
     *          placed <em>after</em> the call to {@code update}, to ensure that the
     *          array slot is not nulled out by {@link Object#finalize} before the
     *          update, even if the call to {@code action} was the last use of this
     *          object. This might be the case if, for example a usage in a user
     *          program
     *          had the form {@code new Resource().action();} which retains no other
     *          reference to this {@code Resource}. While probably overkill here,
     *          {@code reachabilityFence} is placed in a {@code finally} block to
     *          ensure
     *          that it is invoked across all paths in the method. In a method with
     *          more
     *          complex control paths, you might need further precautions to ensure
     *          that
     *          {@code reachabilityFence} is encountered along all of them.
     *
     *          <p>
     *          It is sometimes possible to better encapsulate use of
     *          {@code reachabilityFence}. Continuing the above example, if it were
     *          acceptable for the call to method {@code update} to proceed even if
     *          the
     *          finalizer had already executed (nulling out slot), then you could
     *          localize use of {@code reachabilityFence}:
     *
     *          <pre> {@code
     * public void action2() {
     *   // ...
     *   Resource.update(getExternalResource());
     * }
     * private ExternalResource getExternalResource() {
     *   ExternalResource ext = externalResourceArray[myIndex];
     *   Reference.reachabilityFence(this);
     *   return ext;
     * }}</pre>
     *
     *          <p>
     *          Method {@code reachabilityFence} is not required in constructions
     *          that themselves ensure reachability. For example, because objects
     *          that
     *          are locked cannot, in general, be reclaimed, it would suffice if all
     *          accesses of the object, in all methods of class {@code Resource}
     *          (including {@code finalize}) were enclosed in
     *          {@code synchronized (this)}
     *          blocks. (Further, such blocks must not include infinite loops, or
     *          themselves be unreachable, which fall into the corner case
     *          exceptions to
     *          the "in general" disclaimer.) However, method
     *          {@code reachabilityFence}
     *          remains a better option in cases where this approach is not as
     *          efficient,
     *          desirable, or possible; for example because it would encounter
     *          deadlock.
     *
     * @param ref the reference. If {@code null}, this method has no effect.
     * @since 9
     * @jls 12.6 Finalization of Class Instances
     */
    @ForceInline
    public static void reachabilityFence(Object ref) {
        // Does nothing. This method is annotated with @ForceInline to eliminate
        // most of the overhead that using @DontInline would cause with the
        // HotSpot JVM, when this fence is used in a wide variety of situations.
        // HotSpot JVM retains the ref and does not GC it before a call to
        // this method, because the JIT-compilers do not have GC-only safepoints.
    }
}
