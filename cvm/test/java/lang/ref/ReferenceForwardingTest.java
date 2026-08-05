/*
 * @test
 * @summary Verify that a mixed pending list forwards final and phantom references
 * @run main/othervm -cvm -Xmx64m -XX:+UseParallelGC ReferenceForwardingTest
 * @run main/othervm -cvm -Xmx64m -XX:+UseParallelGC -XX:-UseCompressedOops ReferenceForwardingTest
 * @run main/othervm -cvm -Xmx64m -XX:+UseG1GC ReferenceForwardingTest
 */

import java.lang.ref.PhantomReference;
import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicIntegerArray;
import java.util.concurrent.atomic.AtomicReference;

public class ReferenceForwardingTest {
    private static final int REFERENCE_COUNT = 64;
    private static final long TIMEOUT_MILLIS = 30_000L;

    private static final AtomicInteger finalizedCount = new AtomicInteger();
    private static final AtomicIntegerArray finalized =
            new AtomicIntegerArray(REFERENCE_COUNT);
    private static final AtomicReference<Throwable> finalizerFailure =
            new AtomicReference<Throwable>();

    private static final class FinalizableObject {
        private final int index;

        FinalizableObject(int index) {
            this.index = index;
        }

        @Override
        protected void finalize() {
            if (finalized.incrementAndGet(index) != 1) {
                finalizerFailure.compareAndSet(null,
                        new AssertionError("finalizer ran more than once: " + index));
            }
            finalizedCount.incrementAndGet();
        }
    }

    public static void main(String[] args) throws Exception {
        ReferenceQueue<Object> queue = new ReferenceQueue<Object>();
        List<PhantomReference<Object>> phantomReferences =
                createUnreachableReferences(queue);
        Set<Reference<?>> dequeued = new HashSet<Reference<?>>();

        long deadline = System.currentTimeMillis() + TIMEOUT_MILLIS;
        while ((finalizedCount.get() < REFERENCE_COUNT
                || dequeued.size() < REFERENCE_COUNT)
                && System.currentTimeMillis() < deadline) {
            System.gc();
            System.runFinalization();
            drain(queue, dequeued);
            Thread.sleep(20L);
        }
        drain(queue, dequeued);

        Throwable failure = finalizerFailure.get();
        if (failure != null) {
            throw new AssertionError("Finalizer processing failed", failure);
        }
        if (finalizedCount.get() != REFERENCE_COUNT) {
            throw new AssertionError("Expected " + REFERENCE_COUNT
                    + " finalizers, got " + finalizedCount.get());
        }
        if (dequeued.size() != REFERENCE_COUNT) {
            throw new AssertionError("Expected " + REFERENCE_COUNT
                    + " phantom references, got " + dequeued.size());
        }
        if (!dequeued.containsAll(phantomReferences)) {
            throw new AssertionError("ReferenceQueue lost phantom references");
        }
        for (int index = 0; index < REFERENCE_COUNT; index++) {
            if (finalized.get(index) != 1) {
                throw new AssertionError("Finalizer count for " + index
                        + " is " + finalized.get(index));
            }
        }
    }

    private static List<PhantomReference<Object>> createUnreachableReferences(
            ReferenceQueue<Object> queue) {
        Object[] phantomReferents = new Object[REFERENCE_COUNT];
        FinalizableObject[] finalizableObjects =
                new FinalizableObject[REFERENCE_COUNT];
        List<PhantomReference<Object>> references =
                new ArrayList<PhantomReference<Object>>(REFERENCE_COUNT);

        for (int index = 0; index < REFERENCE_COUNT; index++) {
            phantomReferents[index] = new Object();
            references.add(new PhantomReference<Object>(
                    phantomReferents[index], queue));
            if (references.get(index).get() != null) {
                throw new AssertionError("PhantomReference.get() must return null");
            }
            finalizableObjects[index] = new FinalizableObject(index);
        }
        return references;
    }

    private static void drain(ReferenceQueue<Object> queue,
                              Set<Reference<?>> dequeued) {
        Reference<?> reference;
        while ((reference = queue.poll()) != null) {
            if (!dequeued.add(reference)) {
                throw new AssertionError("Reference was enqueued more than once");
            }
        }
    }
}
