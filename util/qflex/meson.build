qflex_ss.add([files(
  'qflex-log.c',
  'qflex.c',
  'qflex-config.c',
  'qflex-hmp.c',
  'qflex-traces.c',
  'custom-instrumentation.c',
), qflex])

libqflex_ss = ss.source_set()
subdir('libqflex')
qflex_ss.add_all(libqflex_ss)
 