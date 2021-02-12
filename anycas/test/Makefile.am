
AM_CXXFLAGS =  -fdata-sections -ffunction-sections -pthread $(GST_CFLAGS) \
              -I${srcdir}/../include -I${srcdir}/../src/include -I=/usr/include/opencdm

bin_PROGRAMS=demoCasTest
demoCasTest_SOURCES= demoCasTest.cpp

demoCasTest_LDFLAGS= $(LDFLAGS) $(abs_top_builddir)/anycas/libocdmmock.la $(abs_top_builddir)/anycas/libanycas.la -lrdkloggers -llog4c -ljsoncpp
