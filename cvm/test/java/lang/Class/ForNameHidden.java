
/*
 * @test
 * @bug 1234567
 * @summary use Class.forName to detect anonymous classes
 * @run main/othervm -server17 -Xlog:class+init=info,class+load=info ForNameHidden
 * @run main/othervm -server -verbose:class ForNameHidden
 */

import java.util.LinkedList;
import java.util.List;

public class ForNameHidden {

    public static void main(String argv[]) throws Exception {
        List<String> l = new LinkedList<>();
        System.getProperties().forEach((k,v)->{
            if (!l.stream().anyMatch(s-> s.equals(k))) {
                l.add((String)k);
            }
        });

        try {
            Class.forName("ForNameHidden$$Lambda$1");
            throw new RuntimeException("Should throw ClassNotFoundException");
        } catch (ClassNotFoundException e) {
            // expected
        }
    }
}
