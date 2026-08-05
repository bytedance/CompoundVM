/*
 * @test
 * @summary Verify DirectByteBuffer Cleaners release native memory and forward phantom references
 * @run main/othervm -cvm -Xmx64m -XX:MaxDirectMemorySize=16m -XX:+UseParallelGC DirectBufferCleanerTest
 * @run main/othervm -cvm -Xmx64m -XX:MaxDirectMemorySize=16m -XX:+UseG1GC DirectBufferCleanerTest
 */

import java.lang.management.BufferPoolMXBean;
import java.lang.management.ManagementFactory;
import java.lang.ref.PhantomReference;
import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class DirectBufferCleanerTest {
    private static final int MAX_DIRECT_MEMORY = 16 * 1024 * 1024;
    private static final int BUFFER_COUNT = 8;
    private static final long TIMEOUT_MILLIS = 30_000L;

    public static void main(String[] args) throws Exception {
        BufferPoolMXBean directPool = findDirectPool();
        long baselineCapacity = directPool.getTotalCapacity();
        int availableCapacity = (int) (MAX_DIRECT_MEMORY - baselineCapacity);
        if (availableCapacity < BUFFER_COUNT) {
            throw new AssertionError("Insufficient direct-memory budget: baseline="
                    + baselineCapacity);
        }

        ReferenceQueue<ByteBuffer> queue = new ReferenceQueue<ByteBuffer>();
        List<PhantomReference<ByteBuffer>> references = allocateUntilLimit(
                queue, availableCapacity);
        long allocatedCapacity = directPool.getTotalCapacity();
        if (allocatedCapacity < MAX_DIRECT_MEMORY) {
            throw new AssertionError("Direct-memory limit was not filled: "
                    + allocatedCapacity + "/" + MAX_DIRECT_MEMORY);
        }

        ByteBuffer replacement = ByteBuffer.allocateDirect(availableCapacity);
        replacement.put(0, (byte) 1);

        Set<Reference<?>> dequeued = new HashSet<Reference<?>>();
        long deadline = System.currentTimeMillis() + TIMEOUT_MILLIS;
        while (dequeued.size() < references.size()
                && System.currentTimeMillis() < deadline) {
            drain(queue, dequeued);
            Thread.sleep(20L);
        }
        drain(queue, dequeued);

        if (dequeued.size() != references.size()
                || !dequeued.containsAll(references)) {
            throw new AssertionError("Direct-buffer phantom references were not "
                    + "fully forwarded: " + dequeued.size() + "/"
                    + references.size());
        }
        long currentCapacity = directPool.getTotalCapacity();
        long expectedCapacity = baselineCapacity + availableCapacity;
        if (currentCapacity != expectedCapacity) {
            throw new AssertionError("Direct memory accounting is incorrect: expected="
                    + expectedCapacity + ", current=" + currentCapacity);
        }
        if (replacement.get(0) != (byte) 1
                || replacement.capacity() != availableCapacity) {
            throw new AssertionError("Replacement direct buffer is unusable");
        }
    }

    private static List<PhantomReference<ByteBuffer>> allocateUntilLimit(
            ReferenceQueue<ByteBuffer> queue, int totalCapacity) {
        ByteBuffer[] buffers = new ByteBuffer[BUFFER_COUNT];
        List<PhantomReference<ByteBuffer>> references =
                new ArrayList<PhantomReference<ByteBuffer>>(BUFFER_COUNT);
        int remaining = totalCapacity;

        for (int index = 0; index < BUFFER_COUNT; index++) {
            int bufferCapacity = remaining / (BUFFER_COUNT - index);
            buffers[index] = ByteBuffer.allocateDirect(bufferCapacity);
            buffers[index].put(0, (byte) index);
            buffers[index].put(bufferCapacity - 1, (byte) index);
            references.add(new PhantomReference<ByteBuffer>(buffers[index], queue));
            remaining -= bufferCapacity;
        }
        if (remaining != 0) {
            throw new AssertionError("Did not consume the direct-memory budget");
        }
        return references;
    }

    private static BufferPoolMXBean findDirectPool() {
        for (BufferPoolMXBean pool :
                ManagementFactory.getPlatformMXBeans(BufferPoolMXBean.class)) {
            if ("direct".equals(pool.getName())) {
                return pool;
            }
        }
        throw new AssertionError("Direct buffer pool MXBean not found");
    }

    private static void drain(ReferenceQueue<ByteBuffer> queue,
                              Set<Reference<?>> dequeued) {
        Reference<?> reference;
        while ((reference = queue.poll()) != null) {
            if (!dequeued.add(reference)) {
                throw new AssertionError("Reference was enqueued more than once");
            }
        }
    }
}
