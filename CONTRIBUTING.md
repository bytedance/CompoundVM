# How to contribute

We are happy to accept your patches and contributions to this project. There are
just a few small guidelines you need to follow.

## Contributor License Agreement

Contributions to this project must be accompanied by a Contributor License
Agreement. You (or your employer) retain the copyright to your contribution;
this simply gives us permission to use and redistribute your contributions as
part of the project.

You generally only need to submit a CLA once, so if you've already submitted one
(even if it was for a different project), you probably don't need to do it
again.

## Changes Accepted

Please file issues before doing substantial work; this will ensure that others
don't duplicate the work and that there's a chance to discuss any design issues.

In general the changes in hotspot should be guarded by macro `HOTSPOT_TARGET_CLASSLIB`,
This ensures better clarity and makes it easier to backport upstream changes. For example:

```
#if HOTSPOT_TARGET_CLASSLIB == 8
  // code that targets JDK8
#else
  // Original JVM17 code
#endif
```

## Issues

We use GitHub issues to track public bugs. Please ensure your description is
clear and has sufficient instructions to be able to reproduce the issue.
