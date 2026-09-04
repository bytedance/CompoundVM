/*
 * @test
 * @summary Verify that CVM forwards socket shutdown operations to the OS
 * @run main/othervm -cvm SocketShutdownTest
 */

import java.io.InputStream;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

public class SocketShutdownTest {
    private static final long TIMEOUT_MILLIS = 5000;

    public static void main(String[] args) throws Exception {
        testShutdownOutput();
        testShutdownInputUnblocksRead();
    }

    private static void testShutdownOutput() throws Exception {
        InetAddress loopback = InetAddress.getLoopbackAddress();
        try (ServerSocket listener = new ServerSocket(0, 50, loopback);
             Socket client = new Socket(loopback, listener.getLocalPort());
             Socket peer = listener.accept()) {
            peer.setSoTimeout((int) TIMEOUT_MILLIS);

            client.shutdownOutput();

            int value = peer.getInputStream().read();
            if (value != -1) {
                throw new AssertionError("Expected EOF after shutdownOutput, got " + value);
            }
        }
    }

    private static void testShutdownInputUnblocksRead() throws Exception {
        InetAddress loopback = InetAddress.getLoopbackAddress();
        try (ServerSocket listener = new ServerSocket(0, 50, loopback);
             Socket client = new Socket(loopback, listener.getLocalPort());
             Socket peer = listener.accept()) {
            CountDownLatch readStarted = new CountDownLatch(1);
            AtomicReference<Integer> result = new AtomicReference<Integer>();
            AtomicReference<Throwable> failure = new AtomicReference<Throwable>();
            InputStream input = client.getInputStream();

            Thread reader = new Thread(new Runnable() {
                @Override
                public void run() {
                    readStarted.countDown();
                    try {
                        result.set(input.read());
                    } catch (Throwable t) {
                        failure.set(t);
                    }
                }
            }, "socket-shutdown-reader");
            reader.start();

            if (!readStarted.await(TIMEOUT_MILLIS, TimeUnit.MILLISECONDS)) {
                throw new AssertionError("Reader did not start");
            }
            Thread.sleep(100);
            client.shutdownInput();

            reader.join(TIMEOUT_MILLIS);
            if (reader.isAlive()) {
                client.close();
                reader.join(TIMEOUT_MILLIS);
                throw new AssertionError("shutdownInput did not unblock a pending read");
            }
            if (failure.get() != null) {
                throw new AssertionError("Pending read failed after shutdownInput", failure.get());
            }
            if (result.get() == null || result.get().intValue() != -1) {
                throw new AssertionError("Expected EOF after shutdownInput, got " + result.get());
            }
        }
    }
}
