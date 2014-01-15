lib_LTLIBRARIES = libmmp.la

#libmmp_la_SOURCES = libmmp.h \
#pmx.cpp pmx.h vmd.cpp vmd.h \
#motioncontroller.cpp motioncontroller.h interpolation.cpp interpolation.h \
#bulletphysics.cpp bulletphysics.h bulletdebugdrawer.h bulletdebugdrawer.cpp mmdphysics.cpp mmdphysics.h \
#texthandle.cpp texthandle.h Converter.cpp Converter.h shader.cpp shader.h glfw_func_callbacks.cpp glfw_func_callbacks.h

libmmp_la_SOURCES = \
pmx.cpp vmd.cpp \
motioncontroller.cpp interpolation.cpp \
bulletphysics.cpp bulletdebugdrawer.cpp mmdphysics.cpp \
texthandle.cpp Converter.cpp shader.cpp glfw_func_callbacks.cpp

#libmmp_la_LDADD = -lglfw -lGLU -lGLEW -lGL -lSOIL -lBulletDynamics -lBulletCollision -lLinearMath -lBulletSoftBody
#libmmp_la_LIBADD = ../common/libjupcommon.a
libmmp_la_CPPFLAGS = -I$(top_srcdir)/include -I/usr/include/bullet -std=c++0x
