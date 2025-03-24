package jdk.jfr.internal.dcmd;

class DCmdAll {
    private String exampleFilename() {
        if ("\r\n".equals(System.lineSeparator())) {
            return "C:\\Users\\user\\recording.jfr";
        } else {
            return "/recordings/recording.jfr";
        }
    }

    public Argument[] DCmdCheckGetArgumentInfos() {
        return new Argument[] {
            new Argument("name",
                "Recording name, e.g. \\\"My Recording\\\" or omit to see all recordings",
                "STRING", false, null, false),
            new Argument("verbose",
                "Print event settings for the recording(s)","BOOLEAN",
                false, "false", false)
        };
    }

    public Argument[] DCmdDumpGetArgumentInfos() {
        return new Argument[] {
           new Argument("name",
               "Recording name, e.g. \\\"My Recording\\\"",
               "STRING", false, null, false),
           new Argument("filename",
               "Copy recording data to file, e.g. \\\"" + exampleFilename() + "\\\"",
               "STRING", false, null, false),
           new Argument("maxage",
               "Maximum duration to dump, in (s)econds, (m)inutes, (h)ours, or (d)ays, e.g. 60m, or 0 for no limit",
               "NANOTIME", false, null, false),
           new Argument("maxsize", "Maximum amount of bytes to dump, in (M)B or (G)B, e.g. 500M, or 0 for no limit",
               "MEMORY SIZE", false, "hotspot-pid-xxxxx-id-y-YYYY_MM_dd_HH_mm_ss.jfr", false),
           new Argument("begin",
               "Point in time to dump data from, e.g. 09:00, 21:35:00, 2018-06-03T18:12:56.827Z, 2018-06-03T20:13:46.832, -10m, -3h, or -1d",
               "STRING", false, null, false),
           new Argument("end",
               "Point in time to dump data to, e.g. 09:00, 21:35:00, 2018-06-03T18:12:56.827Z, 2018-06-03T20:13:46.832, -10m, -3h, or -1d",
               "STRING", false, null, false),
           new Argument("path-to-gc-roots",
               "Collect path to GC roots",
               "BOOLEAN", false, "false", false)
        };
    }

    public Argument[] DCmdStartGetArgumentInfos() {
        return new Argument[] {
            new Argument("name",
                "Name that can be used to identify recording, e.g. \\\"My Recording\\\"",
                "STRING", false, null, false),
            new Argument("settings",
                "Settings file(s), e.g. profile or default. See JAVA_HOME/lib/jfr",
                "STRING SET", false, "deafult.jfc", true),
            new Argument("delay",
                "Delay recording start with (s)econds, (m)inutes), (h)ours), or (d)ays, e.g. 5h.",
                "NANOTIME", false, "0s", false),
            new Argument("duration",
                "Duration of recording in (s)econds, (m)inutes, (h)ours, or (d)ays, e.g. 300s.",
                "NANOTIME", false, null, false),
            new Argument("disk",
                "Recording should be persisted to disk",
                "BOOLEAN", false, "true", false),
            new Argument("filename",
                "Resulting recording filename, e.g. \\\"" + exampleFilename() +  "\\\"",
                "STRING", false, "hotspot-pid-xxxxx-id-y-YYYY_MM_dd_HH_mm_ss.jfr", false),
            new Argument("maxage",
                "Maximum time to keep recorded data (on disk) in (s)econds, (m)inutes, (h)ours, or (d)ays, e.g. 60m, or 0 for no limit",
                "NANOTIME", false, "0", false),
            new Argument("maxsize",
                "Maximum amount of bytes to keep (on disk) in (k)B, (M)B or (G)B, e.g. 500M, or 0 for no limit",
                "MEMORY SIZE", false, "250M", false),
            new Argument("flush-interval",
                "Dump running recording when JVM shuts down",
                "NANOTIME", false, "1s", false),
            new Argument("dumponexit",
                "Minimum time before flushing buffers, measured in (s)econds, e.g. 4 s, or 0 for flushing when a recording ends",
                "BOOLEAN", false, "false", false),
            new Argument("path-to-gc-roots",
                "Collect path to GC roots",
                "BOOLEAN", false, "false", false)
        };
    }

    public Argument[] DCmdStopGetArgumentInfos() {
        return new Argument[] {
            new Argument("name",
                "Recording text,.e.g \\\"My Recording\\\"",
                "STRING", true, null, false),
            new Argument("filename",
                "Copy recording data to file, e.g. \\\"" + exampleFilename() +  "\\\"",
                "STRING", false, null, false)
        };
    }
}