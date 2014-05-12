
// C:\j2sdk1.4.2\include\jni.h -> C:\Borland\BCC55\Include\jni.h
// C:\j2sdk1.4.2\include\jni_md.h -> C:\Borland\BCC55\Include\jni_md.h

#include <jni.h>

#include <exception>

#ifndef _Included_jnu
#define _Included_jnu

template <class T>
class auto_p_JNI {
private:
	T p;
	JNIEnv* env;
public:
	auto_p_JNI(JNIEnv* env, T p = NULL) : env(env), p(p) { }
	void release() { if (p) { env->DeleteLocalRef(p); p = NULL; } }
	T get() { return p; }
	T set(T p) { if (p != this->p) { release(); return (this->p = p); } return p; }
	~auto_p_JNI() { release(); }
};

class auto_p_JNI_pjbyte {
private:
	jbyte* p;
	jbyteArray arr;
	JNIEnv* env;
	static jbyte* getp(JNIEnv* env, jboolean* hasException, jbyteArray arr) {
		if (arr == NULL)
			return NULL;
		else {
			jbyte* res = env->GetByteArrayElements(arr, NULL);
			if (hasException)
				*hasException = env->ExceptionCheck();
			return res;
		}
	}
public:
	auto_p_JNI_pjbyte(JNIEnv* env, jboolean* hasException = NULL, jbyteArray arr = NULL) : env(env), arr(arr), p(getp(env, hasException, arr)) { }
	void release() { if (p) { env->ReleaseByteArrayElements(arr, p, 0); p = NULL; arr = NULL; } }
	jbyte* get() { return p; }
	jbyte* set(jboolean* hasException, jbyteArray arr) { if (arr != this->arr) { release(); return (p = getp(env, hasException, this->arr = arr)); } return p; }
	~auto_p_JNI_pjbyte() { release(); }
};

class JNU_Exception : std::exception {
public:
	JNU_Exception(const char* msg) : std::exception(msg) {}
	static void check(JNIEnv* env, jboolean* hasException, const char * msg) {
		if(hasException)
			*hasException=env->ExceptionCheck();
		else if(env->ExceptionCheck()){
			env->ExceptionDescribe();
			throw JNU_Exception(msg);
		}
	}
};

JNIEnv* JNU_GetEnv(
  JavaVM* jvm
);

JNIEXPORT void JNU_ThrowByName(
    JNIEnv* env, 
    const char* name, 
    const char* msg
);

JNIEXPORT jvalue JNU_CallMethodByName(
    JNIEnv* env,
    jboolean* hasException,
    jobject obj,
    const char* name,
    const char* descriptor,
    ...
);

JNIEXPORT jvalue JNU_CallStaticMethodByName(
    JNIEnv* env,
    jboolean* hasException,
    jclass clazz,
    const char* name,
    const char* descriptor,
    ...
);

JNIEXPORT jobject JNU_NewObject(
    JNIEnv* env, 
    jboolean* hasException, 
    const char* classname, 
    const char* descriptor, 
    ...
);

JNIEXPORT jstring JNU_NewStringUTF(
    JNIEnv* env, 
    jboolean* hasException, 
    const char* str
);

JNIEXPORT jbyteArray JNU_NewByteArray(
    JNIEnv* env, 
    jboolean* hasException, 
    const jint size, 
    const jbyte* list
);

JNIEXPORT jintArray JNU_NewIntArray(
    JNIEnv* env, 
    jboolean* hasException, 
    const jint size, 
    const jint* list
);

JNIEXPORT jobjectArray JNU_NewStringArray(
    JNIEnv* env, 
    jboolean* hasException, 
    const jint size, 
    const char** list
);

JNIEXPORT void JNU_SetIntField(
    JNIEnv* env, 
    jboolean* hasException, 
    jobject obj,
    const char* name,
    jint value
);

JNIEXPORT void JNU_SignalException(
    JNIEnv* env, 
    jclass jclazz, 
    char* cmsg
);

#endif
