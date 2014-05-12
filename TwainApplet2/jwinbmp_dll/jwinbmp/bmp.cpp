#include "bmp.h"

#define TYPE_CUSTOM             0x00
#define TYPE_INT_RGB            0x01
#define TYPE_INT_ARGB           0x02
#define TYPE_INT_ARGB_PRE       0x03
#define TYPE_INT_BGR            0x04
#define TYPE_3BYTE_BGR          0x05
#define TYPE_4BYTE_ABGR         0x06
#define TYPE_4BYTE_ABGR_PRE     0x07
#define TYPE_USHORT_565_RGB     0x08
#define TYPE_USHORT_555_RGB     0x09
#define TYPE_BYTE_GRAY          0x0A
#define TYPE_USHORT_GRAY        0x0B
#define TYPE_BYTE_BINARY        0x0C
#define TYPE_BYTE_INDEXED       0x0D

static char TwainBufferedImage[] ="uk/co/mmscomputing/device/twain/TwainBufferedImage";

jobject GetIndexColorModel(JNIEnv* env, jboolean* hasException, const jobject bufferedImage, jint* pjMapSize) {
	jobject jIndexColorModel = JNU_CallMethodByName(env, hasException, bufferedImage, "getColorModel", "()Ljava/awt/image/ColorModel;").l;
	if (*hasException) return jIndexColorModel;
	*pjMapSize = JNU_CallMethodByName(env, hasException, jIndexColorModel, "getMapSize", "()I").i;
	return jIndexColorModel;
}

void GetBufferedImageInfo(JNIEnv* env, jboolean* hasException, const jobject bufferedImage, jint* pjType, jint* pjWidth, jint* pjHeight) {
	*pjType = JNU_CallMethodByName(env, hasException, bufferedImage, "getType", "()I").i;
	if (*hasException) return;
	*pjWidth = JNU_CallMethodByName(env, hasException, bufferedImage, "getWidth", "()I").i;
	if (*hasException) return;
	*pjHeight = JNU_CallMethodByName(env, hasException, bufferedImage, "getHeight", "()I").i;
}

jbyteArray GetBufferedImageData(JNIEnv* env, jboolean* hasException, const jobject bufferedImage) {
	auto_p_JNI<jobject> raster(env, JNU_CallMethodByName(env, hasException, bufferedImage, "getData", "()Ljava/awt/image/Raster;").l);
	if (*hasException) return NULL;
	auto_p_JNI<jobject> dataBuffer(env, JNU_CallMethodByName(env, hasException, raster.get(), "getDataBuffer", "()Ljava/awt/image/DataBuffer;").l);
	raster.release();
	if (*hasException) return NULL;
	jbyteArray data = (jbyteArray)(JNU_CallMethodByName(env, hasException, dataBuffer.get(), "getData", "()[B").l);
	dataBuffer.release();
	if (*hasException) return NULL;
	return data;
}

enum COLOR { RED = 0, GREEN = 1, BLUE = 2 };

void SetColor(JNIEnv* env, jboolean* hasException, const jobject jIndexColorModel, const jint size, const COLOR color, BITMAPINFO* res) {
	auto_p_JNI<jbyteArray> col(env, JNU_NewByteArray(env, hasException, size, NULL));
	if (*hasException) return;
	const char* func[] = { "getReds", "getGreens", "getBlues" };
	JNU_CallMethodByName(env, hasException, jIndexColorModel, func[color], "([B)V", col);
	if (*hasException) return;
	auto_p_JNI_pjbyte c(env, hasException, col.get());
	if (*hasException) return;
	for (WORD i = 0; i < size; i++)
		switch (color) {
		case RED:	res->bmiColors[i].rgbRed = c.get()[i]; break;
		case GREEN:	res->bmiColors[i].rgbGreen = c.get()[i]; break;
		case BLUE:	res->bmiColors[i].rgbBlue = c.get()[i]; break;
	}
	c.release();
	col.release();
}

PBITMAPINFO HeapAllocBITMAPINFO(WORD planes, WORD bpp, LONG width, LONG height, WORD* pcClrBits)
{ 
	PBITMAPINFO pbmi; 
	WORD    cClrBits; 
	// Convert the color format to a count of bits.  
	cClrBits = (WORD)(planes * bpp); 
	if (cClrBits == 1) 
		cClrBits = 1; 
	else if (cClrBits <= 4) 
		cClrBits = 4; 
	else if (cClrBits <= 8) 
		cClrBits = 8; 
	else if (cClrBits <= 16) 
		cClrBits = 16; 
	else if (cClrBits <= 24) 
		cClrBits = 24; 
	else cClrBits = 32; 
	// Allocate memory for the BITMAPINFO structure. (This structure  
	// contains a BITMAPINFOHEADER structure and an array of RGBQUAD  
	// data structures.)  
	if (cClrBits < 24) 
		pbmi = (PBITMAPINFO)HeapAlloc(GetProcessHeap(), 0, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1<<cClrBits)); 
	// There is no RGBQUAD array for these formats: 24-bit-per-pixel or 32-bit-per-pixel 
	else 
		pbmi = (PBITMAPINFO)HeapAlloc(GetProcessHeap(), 0, sizeof(BITMAPINFOHEADER)); 
	if (pbmi == NULL)
		return NULL;
	// Initialize the fields in the BITMAPINFO structure.  
	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
	pbmi->bmiHeader.biWidth = width; 
	pbmi->bmiHeader.biHeight = height; 
	pbmi->bmiHeader.biPlanes = planes; 
	pbmi->bmiHeader.biBitCount = bpp; 
	pbmi->bmiHeader.biClrUsed = (cClrBits < 24 ? 1<<cClrBits : 0); 
	// If the bitmap is not compressed, set the BI_RGB flag.  
	pbmi->bmiHeader.biCompression = BI_RGB; 
	// Compute the number of bytes in the array of color  
	// indices and store the result in biSizeImage.  
	// The width must be DWORD aligned unless the bitmap is RLE 
	// compressed. 
	pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits + 31) & ~31) / 8 * pbmi->bmiHeader.biHeight; 
	// Set biClrImportant to 0, indicating that all of the  
	// device colors are important.  
	pbmi->bmiHeader.biClrImportant = 0; 
	pbmi->bmiHeader.biXPelsPerMeter = 2953;	// 75 dpi
	pbmi->bmiHeader.biYPelsPerMeter = 2953;	// 75 dpi
	if (pcClrBits) // return cClrBits
		*pcClrBits = cClrBits;
	return pbmi; 
 } 

PBITMAPINFO HeapAllocBITMAPINFO_FromBufferedImage(JNIEnv* env, jboolean* hasException, const jobject bufferedImage, jint jType = 0, jint jWidth = 0, jint jHeight = 0) {
	PBITMAPINFO res = NULL;
	if (jType == 0 && jWidth == 0 && jHeight == 0) {
		GetBufferedImageInfo(env, hasException, bufferedImage, &jType, &jWidth, &jHeight);
		if (*hasException) return res;
	}
	auto_p_JNI<jobject> jIndexColorModel(env);
	jint jMapSize;
	WORD bpp, biPlanes = 1;
	switch (jType) {
	case TYPE_BYTE_BINARY:
		jIndexColorModel.set(GetIndexColorModel(env, hasException, bufferedImage, &jMapSize));
		if (*hasException) return res;
		bpp = jMapSize <= 2 ? 1 : jMapSize <= 4 ? 2 : 4;
		break;
	case TYPE_BYTE_INDEXED:
		jIndexColorModel.set(GetIndexColorModel(env, hasException, bufferedImage, &jMapSize));
		if (*hasException) return res;
		bpp = 8;
		break;
	case TYPE_BYTE_GRAY:
		bpp = 8;
		break;
	case TYPE_USHORT_GRAY:
	case TYPE_USHORT_555_RGB:
	case TYPE_USHORT_565_RGB:
		bpp = 16;
		break;
	case TYPE_3BYTE_BGR:
		bpp = 24;
		break;
	case TYPE_4BYTE_ABGR:
		bpp = 32;
		break;
	}

	WORD cClrBits;
	if ((res = HeapAllocBITMAPINFO(biPlanes, bpp, jWidth, jHeight, &cClrBits)) == NULL)
		return NULL;

	if (cClrBits < 24) {
		if (jIndexColorModel.get()) {
			for (WORD i = 0; i < 3; i++) {
				SetColor(env, hasException, jIndexColorModel.get(), 1<<cClrBits, (COLOR)i, res);
				if (*hasException) return res;
			}
			jIndexColorModel.release();
		}
		else
			for (BYTE i = 0; i < (1<<cClrBits); i++) {
				res->bmiColors[i].rgbRed = i;
				res->bmiColors[i].rgbGreen = i;
				res->bmiColors[i].rgbBlue = i;
			}
	}

	return res;
}

inline int GetBppFromGdiplusPixelFormat(Gdiplus::PixelFormat pixelFormat)
{
	// Convert the color format to a count of bits per pixel. 
	return (pixelFormat >> 8) & 0xFF;
}

PBITMAPINFO HeapAllocBITMAPINFO_FromGdiplusBitmap(Gdiplus::Bitmap* bmp)
{ 
	PBITMAPINFO pbmi = NULL; 
	WORD    cClrBits; 
	// Retrieve the bitmap color format, width, and height.  
	int bpp = GetBppFromGdiplusPixelFormat(bmp->GetPixelFormat());
	int planes = 1;

	if ((pbmi = HeapAllocBITMAPINFO(planes, bpp, bmp->GetWidth(), bmp->GetHeight(), &cClrBits)) == NULL)
		return NULL;

	if (cClrBits < 24) {
		INT paletteSize = bmp->GetPaletteSize(); // ColorPalette size in bytes
		auto_p_HeapAlloc<Gdiplus::ColorPalette> pPalette(paletteSize);
		bmp->GetPalette(pPalette.get(), paletteSize);
		for (UINT i = 0; i < pPalette.get()->Count; i++) {
			pbmi->bmiColors[i].rgbRed = ((LPRGBQUAD)(pPalette.get()->Entries + i))->rgbRed;
			pbmi->bmiColors[i].rgbGreen = ((LPRGBQUAD)(pPalette.get()->Entries + i))->rgbGreen;
			pbmi->bmiColors[i].rgbBlue = ((LPRGBQUAD)(pPalette.get()->Entries + i))->rgbBlue;
		}
	}

	return pbmi; 
 } 

inline LONG BMP_cBytesPerLine(LONG width, WORD bpp){
  return ((width*bpp+31)>>5)<<2;   // include DWORD padding
}
inline LONG BMP_cBytesPerLine(const BITMAPINFOHEADER* bih){return BMP_cBytesPerLine(bih->biWidth,bih->biBitCount);}

LONG BMP_jBytesPerLine(LONG width, WORD bpp){
  switch(bpp){
  case  1: return (width+7)>>3;
  case  4: return (width+1)>>1;
  case  8: return width;
  case 16: return width*2;
  case 24: return width*3;
  case 32: return width*4;
  }
  return 0L;
}
inline LONG BMP_jBytesPerLine(const BITMAPINFOHEADER* bih){return BMP_jBytesPerLine(bih->biWidth,bih->biBitCount);}

jbyte* DataTransfer::lockBits(int scanline = -1) {
	UINT width = bmp->GetWidth(), height = scanline < 0 ? bmp->GetHeight() : 1;
	Gdiplus::PixelFormat pixelFormat = bmp->GetPixelFormat();
	if (preallocate && scan0.get() == NULL) {
		LONG stride = BMP_cBytesPerLine(width, GetBppFromGdiplusPixelFormat(pixelFormat));
		if ((bitmapData.Scan0 = scan0.set(stride * height)) == NULL) return NULL;
		bitmapData.PixelFormat = pixelFormat;
		bitmapData.Stride = stride;
		bitmapData.Width = width;
		bitmapData.Height = height;
	}
	Gdiplus::Status status = bmp->LockBits(&Gdiplus::Rect(0, scanline < 0 ? 0 : scanline, width, height),
		(preallocate ? Gdiplus::ImageLockModeUserInputBuf : 0) | Gdiplus::ImageLockModeRead, // ::ImageLockMode
		pixelFormat, &bitmapData);
	return (jbyte*)bitmapData.Scan0;
}

jobject BufferedImage_FromGdiplusBitmap(JNIEnv* env, jclass jclazz, jboolean* hasException, Gdiplus::Bitmap* bmp, BOOL preallocate = TRUE) {
	jobject res = NULL;
	auto_p_HeapAlloc<BITMAPINFO> pbmi;
	if (pbmi.set(HeapAllocBITMAPINFO_FromGdiplusBitmap(bmp)) == NULL) return NULL;

	//Gdiplus::BitmapData bitmapData;
	//auto_p_HeapAlloc<jbyte> scan0;
	//if (preallocate) {
	//	LONG stride = BMP_cBytesPerLine(bmp->GetWidth(), GetBppFromGdiplusPixelFormat(bmp->GetPixelFormat()));
	//	if ((bitmapData.Scan0 = scan0.set(stride * bmp->GetHeight())) == NULL) return NULL;
	//	bitmapData.PixelFormat = bmp->GetPixelFormat();
	//	bitmapData.Stride = stride;
	//	bitmapData.Width = bmp->GetWidth();
	//	bitmapData.Height = bmp->GetHeight();
	//}
	//bmp->LockBits(&Gdiplus::Rect(0, 0, bmp->GetWidth(), bmp->GetHeight()),
	//	(preallocate ? Gdiplus::ImageLockModeUserInputBuf : 0) | Gdiplus::ImageLockModeRead, // ::ImageLockMode
	//	bmp->GetPixelFormat(), &bitmapData);
	//res = BMP_transferImage(env, jclazz, hasException, (PBITMAPINFOHEADER)pbmi.get(), &DataTransfer(bitmapData.Scan0));
	//bmp->UnlockBits(&bitmapData);
	//scan0.release();
	DataTransfer dataTransfer(bmp, TRUE, preallocate);
	res = BMP_transferImage(env, jclazz, hasException, (PBITMAPINFOHEADER)pbmi.get(), &dataTransfer);
	dataTransfer.release();

	pbmi.release();
	if (*hasException) return NULL;
	return res;
}

jbyte* GetBitsFromBufferedImageData(JNIEnv* env, jboolean* hasException, const PBITMAPINFOHEADER pbih, auto_p_JNI<jbyteArray>* jdata,
									auto_p_JNI_pjbyte* data0, auto_p_HeapAlloc<jbyte>* data1) {
	LONG cBytesPerLine = BMP_cBytesPerLine(pbih), jBytesPerLine = BMP_jBytesPerLine(pbih);
	BOOL bufferAtOnce = (cBytesPerLine == jBytesPerLine); // && pbmi.get()->bmiHeader.biBitCount < 32);
	jbyte* data;
	if (bufferAtOnce) {
		data = data0->set(hasException, jdata->get());
		if (*hasException) return NULL;
		if (pbih->biBitCount == 32)
			data++; // for 32-bit convert BGRA (Windows BITMAPINFO RGBQUAD) to ABGR (Java BufferedImage)
	} else {
		jbyte* coffset = data = data1->set(cBytesPerLine * pbih->biHeight);
		jint joffset = 0;
		for (INT i = 0; i < pbih->biHeight; i++) {
			if (pbih->biBitCount < 32)
				env->GetByteArrayRegion(jdata->get(), joffset, jBytesPerLine, coffset);
			else if (pbih->biBitCount == 32)
				env->GetByteArrayRegion(jdata->get(), joffset + 1, jBytesPerLine - 1, coffset);

			if (*hasException = env->ExceptionCheck()) return NULL;

			joffset += jBytesPerLine;
			coffset += cBytesPerLine;
		}
		jdata->release();
	}
	return data;
}

BOOL GdiplusGraphicsNotSupportedImagePixelFormat(Gdiplus::PixelFormat pf) {
	return pf == PixelFormatUndefined
		|| pf == PixelFormatDontCare
		|| pf == PixelFormat1bppIndexed
		|| pf == PixelFormat4bppIndexed
		|| pf == PixelFormat8bppIndexed
		|| pf == PixelFormat16bppGrayScale
		|| pf == PixelFormat16bppARGB1555;
}

jobject ResizeCloneBufferedImage(JNIEnv* env, jclass jclazz, jboolean* hasException, const jobject bufferedImage, INT width, INT height)
{
	jint jType, jWidth, jHeight;
	GetBufferedImageInfo(env, hasException, bufferedImage, &jType, &jWidth, &jHeight);
	if (*hasException) return NULL;

    UINT w = jWidth, h = jHeight,
		cw = width, ch = height;
	double wr = (double)w / (double)cw,
		hr = (double)h / (double)ch;
	if (wr > hr) {
		cw = (UINT)(w / wr);
		ch = (UINT)(h / wr);
	} else {
		cw = (UINT)(w / hr);
		ch = (UINT)(h / hr);
	}

	jobject res = bufferedImage;
	GdiplusToken gdiplusToken;

	if (cw != w || ch != h) {

		auto_p_HeapAlloc<BITMAPINFO> pbmi(HeapAllocBITMAPINFO_FromBufferedImage(env, hasException, bufferedImage, jType, jWidth, jHeight));
		if (*hasException || pbmi.get() == NULL) return NULL;

		auto_p_JNI<jbyteArray> jdata(env, GetBufferedImageData(env, hasException, bufferedImage)); // source jByteArray
		if (*hasException) return NULL;
		auto_p_JNI_pjbyte data0(env); // data0 and data1 are auxiliary auto pointer to destroy the buffer after its use
		auto_p_HeapAlloc<jbyte> data1; // only one of them gets initialized at a time

		//return BMP_transferImage(env, jclazz, hasException, (PBITMAPINFOHEADER)pbmi.get(), GetBitsFromBufferedImageData(env, hasException, (PBITMAPINFOHEADER)pbmi.get(), &jdata, &data0, &data1));

		gdiplusToken.startup();

		//Gdiplus::Bitmap bmp1_(pbmi.get(), GetBitsFromBufferedImageData(env, hasException, (PBITMAPINFOHEADER)pbmi.get(), &jdata, &data0, &data1));
		//Gdiplus::Bitmap* bmp1 = &bmp1_;
		auto_p_new<Gdiplus::Bitmap> bmp1(new Gdiplus::Bitmap(pbmi.get(), GetBitsFromBufferedImageData(env, hasException, (PBITMAPINFOHEADER)pbmi.get(), &jdata, &data0, &data1)));

		//res = BufferedImage_FromGdiplusBitmap(env, jclazz, hasException, bmp1);
		//if (*hasException) return NULL;
		//return res;

		if (GdiplusGraphicsNotSupportedImagePixelFormat(bmp1->GetPixelFormat()))
			bmp1.set(bmp1->Clone(0, 0, w, h, PixelFormat24bppRGB));

		//Gdiplus::Bitmap bmp2_(cw, ch, bmp1->GetPixelFormat());
		//Gdiplus::Bitmap* bmp2 = &bmp2_;
		auto_p_new<Gdiplus::Bitmap> bmp2(new Gdiplus::Bitmap(cw, ch, bmp1->GetPixelFormat()));

		Gdiplus::Status status;
		auto_p_new<Gdiplus::Graphics> graphics(new Gdiplus::Graphics(bmp2));
		status = graphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQuality); // ::InterpolationMode
		status = graphics->SetSmoothingMode(Gdiplus::SmoothingModeHighQuality); // ::SmoothingMode
		status = graphics->SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality); // ::PixelOffsetMode
		status = graphics->SetCompositingQuality(Gdiplus::CompositingQualityHighQuality); // ::CompositingQuality
		//status = graphics->SetTextRenderingHint(Gdiplus::TextRenderingHintSystemDefault); // ::TextRenderingHint
		status = graphics->DrawImage(bmp1, 0, 0, cw, ch);
		if (status == Gdiplus::OutOfMemory) {
			graphics.set(new Gdiplus::Graphics(bmp2));
			status = graphics->DrawImage(bmp1, 0, 0, cw, ch);
		}
		graphics.release();

		bmp1.release();

		pbmi.release();
		data1.release();
		data0.release();
		jdata.release();

		res = BufferedImage_FromGdiplusBitmap(env, jclazz, hasException, bmp2);

		bmp2.release();

		if (*hasException) return NULL;
	}

	gdiplusToken.shutdown();

	return res;
}

jint getColorsInPalette(const BITMAPINFOHEADER* bih){
  if(bih->biClrUsed!=0){           // as in info header
    return bih->biClrUsed;
  }else if(bih->biBitCount<16){    // 2, 16, 256 colours
    return 1<<bih->biBitCount;
  }else{                           // 0 for 16,24,32 bits per pixel
    return 0;
  }
}

jbyte* getImageData(const BITMAPINFOHEADER* bih, const DataTransfer* data){
  jbyte* jbuf=data?data->data?(jbyte*)data->data:NULL:(jbyte*)&((BITMAPINFO*)bih)->bmiColors[getColorsInPalette(bih)];
  if(jbuf==NULL)
    return NULL;
  if(bih->biCompression==BI_BITFIELDS){
    jbuf+=3*sizeof(DWORD);
  }
  return jbuf;
}

void BMP_copy(JNIEnv* env,WORD bpp,jbyteArray jbuf,jint joffset,jint len,const jbyte* coffset){
  if(bpp<32)
    env->SetByteArrayRegion(jbuf,joffset,len,coffset);
  else if(bpp==32)
    env->SetByteArrayRegion(jbuf,joffset+1,len-1,coffset); // for 32-bit convert BGRA (Windows BITMAPINFO RGBQUAD) to ABGR (Java BufferedImage)
}

void BMP_setImage(JNIEnv* env,jclass jclazz,jobject jimage, jboolean* hasException,const BITMAPINFOHEADER* bih,jbyteArray jbuf, jint jBytesPerLine, DataTransfer* data){
  jint   cBytesPerLine=BMP_cBytesPerLine(bih);   // include DWORD padding
  jbyte* coffset=getImageData(bih,data);
  BOOL   flip=(!data||data->flip);
  if(flip||jBytesPerLine!=cBytesPerLine){
    jint joffset=flip?0:(bih->biHeight-1)*jBytesPerLine;
    if(coffset!=NULL)
      coffset+=(bih->biHeight-1)*cBytesPerLine;
    for(int i=bih->biHeight-1;i>=0;i--){
      BMP_copy(env,bih->biBitCount,jbuf,joffset,jBytesPerLine,coffset!=NULL?coffset:data->lockBits(i));
	  if(flip)
        joffset+=jBytesPerLine;
      else
        joffset-=jBytesPerLine;
      if(coffset!=NULL)
        coffset-=cBytesPerLine;
	  else
        data->unlockBits();
      if(hasException)
        if(*hasException=env->ExceptionCheck())
          return;
    }
  }else{
    BMP_copy(env,bih->biBitCount,jbuf,0,bih->biSizeImage,coffset!=NULL?coffset:data->lockBits());
	if(coffset==NULL)
      data->unlockBits();
    if(hasException)
      if(*hasException=env->ExceptionCheck())
        return;
  }
  env->DeleteLocalRef(jbuf);
}

jobject get01BitIndexColorModel(JNIEnv* env, jboolean* hasException,const BITMAPINFOHEADER* bih){
  const  int bitcount=1;
  jbyte  r[1<<bitcount]={0},g[1<<bitcount]={0},b[1<<bitcount]={0};
  const  int maxcol=1<<bitcount;

  for(int i=0;i<maxcol;i++){
    r[i]=((BITMAPINFO*)bih)->bmiColors[i].rgbRed;
    g[i]=((BITMAPINFO*)bih)->bmiColors[i].rgbGreen;
    b[i]=((BITMAPINFO*)bih)->bmiColors[i].rgbBlue;
  }
  jobject jcoltab=JNU_NewObject(env,hasException,"java/awt/image/IndexColorModel","(II[B[B[B)V",
    bitcount,maxcol,
    JNU_NewByteArray(env,hasException,maxcol,(const jbyte*)&r),
    JNU_NewByteArray(env,hasException,maxcol,(const jbyte*)&g),
    JNU_NewByteArray(env,hasException,maxcol,(const jbyte*)&b)
  );
  return jcoltab;
}

jobject BMP_transfer01BitImage(JNIEnv* env, jclass jclazz,jboolean* hasException, const BITMAPINFOHEADER* bih, DataTransfer* data){
  jobject jimage=(jobject)JNU_NewObject(
      env,hasException,
      TwainBufferedImage,
      "(IIILjava/awt/image/IndexColorModel;)V",
      bih->biWidth,
      bih->biHeight,
      TYPE_BYTE_BINARY,
      get01BitIndexColorModel(env,hasException,bih)
  );
  if(jimage!=NULL){
    jbyteArray jbuf=(jbyteArray)(JNU_CallMethodByName(
        env,hasException,
        jimage,
        "getBuffer",
        "()[B"
    ).l);
    if(jbuf!=NULL){
      BMP_setImage(env,jclazz,jimage,hasException,bih,jbuf,(bih->biWidth+7)>>3,data);
    }
  }
  return jimage;
}

jobject get04BitIndexColorModel(JNIEnv* env, jboolean* hasException,const BITMAPINFOHEADER* bih){
  const  int bitcount=4;
  jbyte  r[1<<bitcount]={0},g[1<<bitcount]={0},b[1<<bitcount]={0};
  int    maxcol=getColorsInPalette(bih);

  for(int i=0;i<maxcol;i++){
    r[i]=((BITMAPINFO*)bih)->bmiColors[i].rgbRed;
    g[i]=((BITMAPINFO*)bih)->bmiColors[i].rgbGreen;
    b[i]=((BITMAPINFO*)bih)->bmiColors[i].rgbBlue;
  }
  jobject jcoltab=JNU_NewObject(env,hasException,"java/awt/image/IndexColorModel","(II[B[B[B)V",
    bitcount,maxcol,
    JNU_NewByteArray(env,hasException,maxcol,(const jbyte*)&r),
    JNU_NewByteArray(env,hasException,maxcol,(const jbyte*)&g),
    JNU_NewByteArray(env,hasException,maxcol,(const jbyte*)&b)
  );
  return jcoltab;
}

jobject BMP_transfer04BitImage(JNIEnv* env,jclass jclazz,jboolean* hasException, const BITMAPINFOHEADER* bih, DataTransfer* data){
  jobject jimage=(jobject)JNU_NewObject(
      env,hasException,
      TwainBufferedImage,
      "(IIILjava/awt/image/IndexColorModel;)V",
      bih->biWidth,
      bih->biHeight,
      TYPE_BYTE_BINARY,
      get04BitIndexColorModel(env,hasException,bih)
  );
  if(jimage!=NULL){
    jbyteArray jbuf=(jbyteArray)(JNU_CallMethodByName(
        env,hasException,
        jimage,
        "getBuffer",
        "()[B"
    ).l);
    if(jbuf!=NULL){
      BMP_setImage(env,jclazz,jimage,hasException,bih,jbuf,(bih->biWidth+1)>>1,data);
    }
  }
  return jimage;
}

jobject get08BitIndexColorModel(JNIEnv* env, jboolean* hasException,const BITMAPINFOHEADER* bih){
  const  int bitcount=8;
  jbyte  r[1<<bitcount]={0},g[1<<bitcount]={0},b[1<<bitcount]={0};
  int    maxcol=getColorsInPalette(bih);

  for(int i=0;i<maxcol;i++){
    r[i]=((BITMAPINFO*)bih)->bmiColors[i].rgbRed;
    g[i]=((BITMAPINFO*)bih)->bmiColors[i].rgbGreen;
    b[i]=((BITMAPINFO*)bih)->bmiColors[i].rgbBlue;
  }
  jobject jcoltab=JNU_NewObject(env,hasException,"java/awt/image/IndexColorModel","(II[B[B[B)V",
    bitcount,maxcol,
    JNU_NewByteArray(env,hasException,maxcol,(const jbyte*)&r),
    JNU_NewByteArray(env,hasException,maxcol,(const jbyte*)&g),
    JNU_NewByteArray(env,hasException,maxcol,(const jbyte*)&b)
  );
  return jcoltab;
}

jboolean checkGreyScale(JNIEnv* env, jboolean* hasException,const BITMAPINFOHEADER* bih){
  if(getColorsInPalette(bih)!=256){ return FALSE;}

  RGBQUAD color;
  for(int i=0;i<256;i++){
    color = ((BITMAPINFO*)bih)->bmiColors[i];
    if(color.rgbRed  !=i){ return FALSE;}
    if(color.rgbGreen!=i){ return FALSE;}
    if(color.rgbBlue !=i){ return FALSE;}
  }
  return TRUE;
}

jobject BMP_transfer08BitImage(JNIEnv* env, jclass jclazz,jboolean* hasException, const BITMAPINFOHEADER* bih, DataTransfer* data){
  jobject jimage;
  if(checkGreyScale(env,hasException,bih)){
    jimage=(jobject)JNU_NewObject(
        env,hasException,
        TwainBufferedImage,
        "(III)V",
        bih->biWidth,
        bih->biHeight,
        TYPE_BYTE_GRAY
    );
  }else{
    jimage=(jobject)JNU_NewObject(
        env,hasException,
        TwainBufferedImage,
        "(IIILjava/awt/image/IndexColorModel;)V",
        bih->biWidth,
        bih->biHeight,
        TYPE_BYTE_INDEXED,
        get08BitIndexColorModel(env,hasException,bih)
    );
  }
  if(jimage!=NULL){
    jbyteArray jbuf=(jbyteArray)(JNU_CallMethodByName(
        env,hasException,
        jimage,
        "getBuffer",
        "()[B"
    ).l);
    if(jbuf!=NULL){
      BMP_setImage(env,jclazz,jimage,hasException,bih,jbuf,bih->biWidth,data);
    }
  }
  return jimage;
}

jobject BMP_transfer24BitImage(JNIEnv* env, jclass jclazz,jboolean* hasException, const BITMAPINFOHEADER* bih, DataTransfer* data){
  jobject jimage=(jobject)JNU_NewObject(
      env,hasException,
      TwainBufferedImage,
      "(III)V",
      bih->biWidth,
      bih->biHeight,
      TYPE_3BYTE_BGR
  );
  if(jimage!=NULL){
    jbyteArray jbuf=(jbyteArray)(JNU_CallMethodByName(
        env,hasException,
        jimage,
        "getBuffer",
        "()[B"
    ).l);
    if(jbuf!=NULL){
      BMP_setImage(env,jclazz,jimage,hasException,bih,jbuf,bih->biWidth*3,data);
    }
  }
  return jimage;
}

jobject BMP_transfer32BitImage(JNIEnv* env, jclass jclazz,jboolean* hasException, const BITMAPINFOHEADER* bih, DataTransfer* data){
  jobject jimage=(jobject)JNU_NewObject(
      env,hasException,
      TwainBufferedImage,
      "(III)V",
      bih->biWidth,
      bih->biHeight,
      TYPE_4BYTE_ABGR
  );
  if(jimage!=NULL){
    jbyteArray jbuf=(jbyteArray)(JNU_CallMethodByName(
        env,hasException,
        jimage,
        "getBuffer",
        "()[B"
    ).l);
    if(jbuf!=NULL){
      BMP_setImage(env,jclazz,jimage,hasException,bih,jbuf,bih->biWidth*4,data);
    }
  }
  return jimage;
}

jobject BMP_transferImage(JNIEnv* env, jclass jclazz,jboolean* hasException,/* HGLOBAL dib*/ const BITMAPINFOHEADER* bih, DataTransfer* data){
  jobject jimage=NULL;

  //if(dib!=NULL){
  //  BITMAPINFOHEADER* bih=(BITMAPINFOHEADER*)DSM_Lock(dib);           // get a pointer to the beginning of the DIB
    if(bih!=NULL){ 
      int type=(int)bih->biCompression;

//    fprintf(stderr,"bitCount=%d\ntype=%d\n",bih->biBitCount,bih->biCompression);

      if((type==BI_RGB)||(type==BI_BITFIELDS)){
        switch(bih->biBitCount){
        case  1: jimage=BMP_transfer01BitImage(env,jclazz,hasException,bih,data);break;
        case  4: jimage=BMP_transfer04BitImage(env,jclazz,hasException,bih,data);break;
        case  8: jimage=BMP_transfer08BitImage(env,jclazz,hasException,bih,data);break;
        case 24: jimage=BMP_transfer24BitImage(env,jclazz,hasException,bih,data);break;
        case 32: jimage=BMP_transfer32BitImage(env,jclazz,hasException,bih,data);break;
        default:
          //JNU_SignalException(env,jclazz,"Unsupported biBitCount in DIB.");
          break;
        }
        JNU_CallMethodByName(
          env,hasException,
          jimage,
          "setPixelsPerMeter",
          "(II)V",
          bih->biXPelsPerMeter,
          bih->biYPelsPerMeter
        );
      }else{
        //JNU_SignalException(env,jclazz,"Cannot deal with DIB type [RLE4 or RLE8].");
      }
      //DSM_Unlock(dib);
    }else{
      //JNU_SignalException(env,jclazz,"Cannot get DIB pointer.");
    }
    //DSM_Free(dib);
  //}else{
  //  JNU_SignalException(env,jclazz,"DIB is a NULL pointer.");
  //}
  return jimage;
}
/*
void BMP_writeImage(JNIEnv* env, jclass jclazz,jboolean* hasException, HGLOBAL dib){
  BITMAPFILEHEADER  bfh={0};
  BITMAPINFOHEADER* bih;
  jint              len;
  if(dib!=NULL){
    len=DSM_Size(dib);                              
    bih=(BITMAPINFOHEADER*)DSM_Lock(dib);           // get a pointer to the beginning of the DIB
    if(bih!=NULL){ 
      jbyteArray jbuf=env->NewByteArray(sizeof(BITMAPFILEHEADER)+len);
      if(jbuf!=NULL){
        bfh.bfType=0x4D42;                            // "BM"
        bfh.bfSize=sizeof(BITMAPFILEHEADER)+len;
        bfh.bfOffBits=sizeof(BITMAPFILEHEADER)
                     +sizeof(BITMAPINFOHEADER)
                     +sizeof(RGBQUAD)*getColorsInPalette(bih)
                     +((bih->biCompression==BI_BITFIELDS)?6:0);  // 16 & 32bit
        env->SetByteArrayRegion(jbuf,0,sizeof(BITMAPFILEHEADER),(jbyte*)&bfh);
        env->SetByteArrayRegion(jbuf,sizeof(BITMAPFILEHEADER),len,(jbyte*)bih);
        JNU_CallStaticMethodByName(env,hasException,jclazz,"writeImage","([B)V",jbuf);
        env->DeleteLocalRef(jbuf);
      }else{
        JNU_SignalException(env,jclazz,"Cannot allocate image buffer.");
      }
      DSM_Unlock(dib);
    }else{
      JNU_SignalException(env,jclazz,"Cannot get DIB pointer.");
    }
    DSM_Free(dib);
  }else{
    JNU_SignalException(env,jclazz,"DIB is a NULL pointer.");
  }
}
*/
