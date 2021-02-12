
AUTOMAKE_OPTIONS = subdir-objects

SUBDIRS = . test
DIST_SUBDIRS = 

AM_CXXFLAGS =  -fdata-sections -ffunction-sections -pthread $(GST_CFLAGS) \
              -I${srcdir}/include -I${srcdir}/src/include -I=/usr/include/opencdm \
              ${GLIB_CFLAGS}

lib_LTLIBRARIES = libocdmmock.la libanycas.la

libocdmmock_la_SOURCES =  demoCas/open_cdm_mock.cpp

#libanycas_la_LDFLAGS = $(GST_LIBS) $(GST_BASE_LIBS) -lrdkloggers -llog4c -locdm
libanycas_la_LDFLAGS = $(GST_LIBS) $(GST_BASE_LIBS) -lrdkloggers -llog4c -locdmmock -lglib-2.0 -ljsoncpp
libanycas_la_SOURCES =  src/CASManagerImpl.cpp \
                        src/CASHelper.cpp \
                        src/CASHelperEngineImpl.cpp \
                        src/CASSystemImpl.cpp \
                        src/CASSystemFactory.cpp \
                        src/CASSessionImpl.cpp \
                        src/CASSectionFilterParam.cpp \
                        src/CASSectionFilterImpl.cpp \
                        demoCas/demoCas.cpp \
                        demoCas/demoCas.h \
                        include/CASManager.h \
                        include/CASHelper.h \
                        include/CASHelperFactory.h \
                        include/CASHelperEngine.h \
                        include/CASHelperContext.h \
                        include/CASSystem.h \
                        include/CASSession.h \
                        include/CASSectionFilter.h \
                        include/CASSectionFilterParam.h \
                        include/CASEnvironment.h \
                        include/CASDataListener.h \
                        include/CASSessionStatus.h \
                        include/CASStatus.h \
                        include/CASPipelineController.h

libanycas_la_includedir = $(includedir)
libanycas_la_include_HEADERS = src/include/CASManagerImpl.h \
                        src/include/CASHelperEngineImpl.h \
                        src/include/CASSystemImpl.h \
                        src/include/CASSessionImpl.h \
                        src/include/CASSystemFactory.h \
                        src/include/CASSectionFilterImpl.h \
                        src/include/CASSectionFilterResponse.h \
                        src/include/ICasSectionFilter.h \
                        src/include/ICasHandle.h \
                        src/include/ICasStatusReport.h \
                        src/include/ICasPipeline.h \
                        include/CASManager.h \
                        include/CASHelper.h \
                        include/CASHelperFactory.h \
                        include/CASHelperEngine.h \
                        include/CASHelperContext.h \
                        include/CASSystem.h \
                        include/CASSession.h \
                        include/CASSectionFilter.h \
                        include/CASSectionFilterParam.h \
                        include/CASEnvironment.h \
                        include/CASDataListener.h \
                        include/CASSessionStatus.h \
                        include/CASStatus.h \
                        include/CASPipelineController.h
