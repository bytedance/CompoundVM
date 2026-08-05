/*
 * @test
 * @summary Verify Cleaner dispatch does not break PhantomReference forwarding
 * @run main/othervm -cvm -Xmx64m -XX:+UseParallelGC CleanerReferenceForwardingTest
 * @run main/othervm -cvm -Xmx64m -XX:+UseG1GC CleanerReferenceForwardingTest
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

import sun.misc.Cleaner;

public class CleanerReferenceForwardingTest {
    private static final int REFERENCE_COUNT = 64;
    private static final long TIMEOUT_MILLIS = 30_000L;

    public static void main(String[] args) throws Exception {
        final AtomicInteger cleanedCount = new AtomicInteger();
        final AtomicIntegerArray cleaned = new AtomicIntegerArray(REFERENCE_COUNT);
        ReferenceQueue<Object> queue = new ReferenceQueue<Object>();
        List<PhantomReference<Object>> phantomReferences =
                new ArrayList<PhantomReference<Object>>(REFERENCE_COUNT);
        List<Cleaner> cleaners = createUnreachableReferences(
                queue, phantomReferences, cleanedCount, cleaned);
        Set<Reference<?>> dequeued = new HashSet<Reference<?>>();

        long deadline = System.currentTimeMillis() + TIMEOUT_MILLIS;
        while ((cleanedCount.get() < REFERENCE_COUNT
                || dequeued.size() < REFERENCE_COUNT)
                && System.currentTimeMillis() < deadline) {
            System.gc();
            drain(queue, dequeued);
            Thread.sleep(20L);
        }
        drain(queue, dequeued);

        if (cleanedCount.get() != REFERENCE_COUNT) {
            throw new AssertionError("Expected " + REFERENCE_COUNT
                    + " cleaner actions, got " + cleanedCount.get());
        }
        if (dequeued.size() != REFERENCE_COUNT
                || !dequeued.containsAll(phantomReferences)) {
            throw new AssertionError("Phantom references were not fully forwarded: "
                    + dequeued.size() + "/" + REFERENCE_COUNT);
        }

        for (Cleaner cleaner : cleaners) {
            cleaner.clean();
        }
        if (cleanedCount.get() != REFERENCE_COUNT) {
            throw new AssertionError("Cleaner action ran more than once");
        }
        for (int index = 0; index < REFERENCE_COUNT; index++) {
            if (cleaned.get(index) != 1) {
                throw new AssertionError("Cleaner count for " + index
                        + " is " + cleaned.get(index));
            }
        }
    }

    private static List<Cleaner> createUnreachableReferences(
            ReferenceQueue<Object> queue,
            List<PhantomReference<Object>> phantomReferences,
            final AtomicInteger cleanedCount,
            final AtomicIntegerArray cleaned) {
        Object[] cleanerReferents = new Object[REFERENCE_COUNT];
        Object[] phantomReferents = new Object[REFERENCE_COUNT];
        List<Cleaner> cleaners = new ArrayList<Cleaner>(REFERENCE_COUNT);

        for (int index = 0; index < REFERENCE_COUNT; index++) {
            final int cleanerIndex = index;
            cleanerReferents[index] = new Object();
            cleaners.add(Cleaner.create(cleanerReferents[index], new Runnable() {
                @Override
                public void run() {
                    cleaned.incrementAndGet(cleanerIndex);
                    cleanedCount.incrementAndGet();
                }
            }));

            phantomReferents[index] = new Object();
            phantomReferences.add(new PhantomReference<Object>(
                    phantomReferents[index], queue));
        }
        return cleaners;
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
