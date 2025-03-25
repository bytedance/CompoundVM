public class Loop {

  final static Loop liveInstance = new Loop();

  public static void main (String[] args) {
    while (true) {
      try {
        System.out.println("Wake up");
        Thread.sleep(500);
      } catch(Exception e){
        e.printStackTrace();
      }
    }
  }
}
