// jwinbmp.cpp : Defines the exported functions for the DLL application.
//

#include "jwinbmp.h"
#include "bmp.h"

/*
 * Class:     jwinbmp
 * Method:    resize
 * Signature: (Ljava/awt/image/BufferedImage;II)Ljava/awt/image/BufferedImage;
 */
JNIEXPORT jobject JNICALL Java_jwinbmp_resize(JNIEnv * env, jclass jclazz, jobject bufferedImage, jint width, jint height) {
	jboolean hasException;
	return ResizeCloneBufferedImage(env, jclazz, &hasException, bufferedImage, width, height);
}
