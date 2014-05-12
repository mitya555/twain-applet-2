#include <Windows.h>
#include <GdiPlus.h>
#include "jnu.h"

jobject ResizeCloneBufferedImage(JNIEnv* env, jclass jclazz, jboolean* hasException, const jobject bufferedImage, INT width, INT height);

template <class T>
class auto_p_HeapAlloc {
private:
	T* p;
	inline static T* heapAlloc(SIZE_T size) { return (T*)HeapAlloc(GetProcessHeap(), 0, size); }
	inline static BOOL heapFree(VOID* p) { return HeapFree(GetProcessHeap(), 0, p); }
public:
	auto_p_HeapAlloc(VOID* p = NULL) : p((T*)p) { }
	auto_p_HeapAlloc(SIZE_T size) : p(heapAlloc(size)) { }
	void release() { if (p) { heapFree(p); p = NULL; } }
	inline T* get() const { return p; }
	inline T* operator->() const { return p; }
	inline operator T*() const { return p; }
	T* set(VOID* p) { if ((T*)p != this->p) { release(); return (this->p = (T*)p); } return (T*)p; }
	T* set(SIZE_T size) { release(); return (p = heapAlloc(size)); }
	~auto_p_HeapAlloc() { release(); }
};

template <class T>
class auto_p_new {
private:
	T* p;
public:
	auto_p_new(T* p = NULL) : p(p) { }
	void release() { if (p) { delete p; p = NULL; } }
	inline T* get() const { return p; }
	inline T* operator->() const { return p; }
	inline operator T*() const { return p; }
	T* set(T* p) { if (p != this->p) { release(); return (this->p = p); } return p; }
	~auto_p_new() { release(); }
};

class GdiplusToken {
private:
	ULONG_PTR gdiplusToken;
public:
	GdiplusToken(BOOL startup = FALSE) : gdiplusToken(NULL) { if (startup) this->startup(); }
	Gdiplus::Status startup() { Gdiplus::GdiplusStartupInput gdiplusStartupInput; return Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL); }
	void shutdown() { if (gdiplusToken) { Gdiplus::GdiplusShutdown(gdiplusToken); gdiplusToken = NULL; } }
	~GdiplusToken() { shutdown(); }
};

class DataTransfer {
private:
	Gdiplus::BitmapData bitmapData;
	auto_p_HeapAlloc<jbyte> scan0;
	Gdiplus::Bitmap* bmp;
	const BOOL preallocate;
public:
	const VOID* data;
	const BOOL flip;
	DataTransfer(const VOID* data, BOOL flip = TRUE) : data(data), flip(flip), bmp(NULL), preallocate(NULL) {}
	DataTransfer(Gdiplus::Bitmap* bmp, BOOL flip = TRUE, BOOL preallocate = TRUE) : data(NULL), flip(flip), bmp(bmp), preallocate(preallocate) {}
	jbyte* lockBits(int scanline);
	void unlockBits() { bmp->UnlockBits(&bitmapData); }
	void release() { scan0.release(); }
};

jobject BMP_transferImage(JNIEnv* env, jclass jclazz,jboolean* hasException,/* HGLOBAL dib*/ const BITMAPINFOHEADER* bih, DataTransfer* data);
