
import java.applet.Applet;
//import java.awt.AlphaComposite;
import java.awt.Component;
import java.awt.Graphics2D;
import java.awt.GridLayout;
//import java.awt.Image;
import java.awt.RenderingHints;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.awt.image.BufferedImage;
import java.awt.image.IndexColorModel;
//import java.awt.image.DataBuffer;
import java.awt.image.RenderedImage;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
//import java.io.BufferedReader;
import java.io.BufferedReader;
//import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.ProtocolException;
import java.net.URL;
import java.util.AbstractMap;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
//import java.io.InputStreamReader;
//import java.io.OutputStreamWriter;
//import java.net.URL;
//import java.net.URLConnection;
//import java.net.URLEncoder;



import javax.imageio.ImageIO;
//import javax.swing.Box;
import javax.swing.BorderFactory;
import javax.swing.ImageIcon;
import javax.swing.JButton;
import javax.swing.JCheckBox;

import netscape.javascript.JSObject;
import uk.co.mmscomputing.device.scanner.Scanner;
import uk.co.mmscomputing.device.scanner.ScannerIOMetadata;
import uk.co.mmscomputing.device.twain.TwainBufferedImage;
import uk.co.mmscomputing.device.twain.TwainIOException;
import uk.co.mmscomputing.device.twain.TwainIOMetadata;
import uk.co.mmscomputing.device.twain.TwainImageInfo;
import uk.co.mmscomputing.device.twain.TwainSource;
import uk.co.mmscomputing.device.twain.TwainPanel;
import uk.co.mmscomputing.device.twain.TwainTransfer.MemoryTransfer.Info;

@SuppressWarnings("serial")
public class TwainPanelUpload extends TwainPanel {

	private static boolean DEBUG = true;
	
	private static void error(String msg) {
		if (DEBUG)
			System.err.println(msg);		
	}
	private static void debug(String msg) {
		if (DEBUG)
			System.out.println(msg);		
	}
	private static void debug_(String msg) {
		if (DEBUG)
			System.out.print(msg);		
	}
		
	private static boolean objEmptyString(Object obj) {
		return obj == null || obj.toString().length() == 0;
	}
	private static boolean strNotEmpty(String str) {
		return str != null && str.length() != 0;
	}

	private static class HttpPost {
		private static final String crlf = "\r\n";
		private static final String twoHyphens = "--";
		private static final String boundary = "*****";
		private HttpURLConnection conn = null;
		private DataOutputStream wr = null;
		private boolean hasParam = false;
		public HttpPost(URL url, String cookie) throws IOException {
			debug("Connect to: " + url);
			conn = (HttpURLConnection)url.openConnection();
			conn.setDoOutput(true);
			conn.setRequestMethod("POST");
			conn.setRequestProperty("Connection", "Keep-Alive");
			conn.setRequestProperty("Cache-Control", "no-cache");
			conn.setRequestProperty("Content-Type", "multipart/form-data; boundary=" + boundary);
			String cookie_ = conn.getRequestProperty("Cookie");
			debug("Cookie(auto preset): " + cookie_);
			if (!strNotEmpty(cookie_)) {
				debug("Cookie(from server): " + cookie);
				if (strNotEmpty(cookie))
					conn.setRequestProperty("Cookie", cookie);
			}
			wr = new DataOutputStream(conn.getOutputStream());
		}
		public DataOutputStream addParam(String name, String filename, String contenttype) throws IOException {
			if (hasParam)
				wr.writeBytes(crlf);				
			wr.writeBytes(twoHyphens + boundary + crlf);
			wr.writeBytes("Content-Disposition: form-data; name=\"" + name + "\"" +
					(strNotEmpty(filename) && strNotEmpty(contenttype) ?
							"; filename=\"" + filename + "\"" + crlf +
							"Content-Type: " + contenttype + crlf +
							"Content-Transfer-Encoding: binary" + crlf :
								crlf));
			wr.writeBytes(crlf);
			hasParam = true;
			return wr;
		}
		public DataOutputStream addParam(String name) throws IOException {
			return addParam(name, null, null);
		}
		public StringBuilder send(boolean getresponse, StringBuilder response) throws IOException {
			if (hasParam)
				wr.writeBytes(crlf);				
			wr.writeBytes(twoHyphens + boundary + twoHyphens + crlf);
			wr.flush();
			wr.close();
			debug("Response: " + conn.getResponseCode() + " " + conn.getResponseMessage());
			BufferedReader rd = new BufferedReader(new InputStreamReader(conn.getInputStream()));
			String line;
			debug("Response Body:");
			while ((line = rd.readLine()) != null) {
				if (getresponse)
					response.append(line);
				debug(line);
			}
			rd.close();
			return response;
		}
	}

	private static boolean isYes(String value) {
		return "Yes".equalsIgnoreCase(value) || "True".equalsIgnoreCase(value);
	}
	private static boolean isYes(Object value) {
		if (value instanceof String)
			return isYes((String)value);
		else // if (value instanceof Boolean)
			return (Boolean)value;
	}
	private static boolean isYes(Applet applet, String name) {
		String value = applet.getParameter(name);
		return isYes(value);
	}
	private static boolean isNo(Applet applet, String name) {
		String value = applet.getParameter(name);
		return strNotEmpty(value) && ("No".equalsIgnoreCase(value) || "False".equalsIgnoreCase(value));
	}
	
	private static int getIntParam(Applet applet, String name, final int defval) {
		try {
			return Integer.parseInt(applet.getParameter(name));
		} catch (NumberFormatException e) {
			return defval;
		}
	}
	
	private static URL getUrlParam(Applet applet, String name, final URL defval) {
		try {
			if (strNotEmpty(applet.getParameter(name)))
				return new URL(applet.getDocumentBase(), applet.getParameter(name));
			else
				return defval;
		} catch (MalformedURLException e) {
			e.printStackTrace();
			return defval;
		}
	}

	private int getComponentIndex(Component component) {
		for (int i = 0; i < getComponentCount(); i++)
			if (getComponent(i) == component)
				return i;
		return -1;
	}

	private void imgButton(JButton jButton, final URL imgUrl) {
		if (imgUrl != null) {
//			try {
//				debug("Button Image URL: " + imgUrl);
//				jButton.setIcon(new ImageIcon(ImageIO.read(imgUrl)));
//			} catch (IOException e) {
//				e.printStackTrace();
//			}
			String[] tmp = imgUrl.getFile().split("/");
			String imageName = tmp[tmp.length - 1];
			URL imgUrl_ = getClass().getClassLoader().getResource(imageName);
			debug("Button Image URL: " + imgUrl_);
			jButton.setIcon(new ImageIcon(imgUrl_));
		}
		jButton.setBorder(BorderFactory.createEmptyBorder());
		jButton.setContentAreaFilled(false);
		jButton.setFocusable(false);
	}
	
	public TwainPanelUpload(Scanner scanner, Applet applet) throws TwainIOException {

		super(scanner, getIntParam(applet, "Layout", 4));

		if (isNo(applet, "Debug"))
			DEBUG = false;
		
		try {
			this.jsWindow = JSObject.getWindow(applet);
		} catch (Throwable t) {}
		this.documentBase = applet.getDocumentBase();

		this.imageHandlerUrl = getUrlParam(applet, "ImageHandler", null);
		this.jsCallbackMethod = applet.getParameter("JSCallback");
		this.jsBeforeScanCallbackMethod = applet.getParameter("JSBeforeScanCallback");
		this.jsBeforeSendCallbackMethod = applet.getParameter("JSBeforeSendCallback");
		this.jsErrorCallbackMethod = applet.getParameter("JSErrorCallback");
		this.jsBeforeAcquireCallbackMethod = applet.getParameter("JSBeforeAcquireCallback");
		this.cookie = applet.getParameter("Cookie");
		this.passJsonResult = isYes(applet, "PassJsonResult");
		this.callServerOnAcquire = isYes(applet, "CallServerOnAcquire");
		
		final int horizontalSpacing = getIntParam(applet, "HorizontalSpacing", 0);
		final boolean hideGui = isYes(applet, "HideGui"),
				hideCheckbox = isYes(applet, "HideCheckbox");
		final String jsGuiCheckedChangeMethod = applet.getParameter("JSCheckedChange"),
				jsGuiEnabledChangeMethod = applet.getParameter("JSEnabledChange"),
				acquireLabel = applet.getParameter("AcquireLabel"),
				selectLabel = applet.getParameter("SelectLabel"),
				acquireToolTip = applet.getParameter("AcquireToolTip"),
				selectToolTip = applet.getParameter("SelectToolTip");

		final URL acquireImgUrl = getUrlParam(applet, "AcquireImgUrl", null),
				selectImgUrl = getUrlParam(applet, "SelectImgUrl", null);

		for (Component ctrl : getComponents())
			if (ctrl instanceof JCheckBox) {
				guicheckbox = (JCheckBox)ctrl;
				if (hideGui)
					guicheckbox.setVisible(false);
				else if (hideCheckbox)
					this.remove(guicheckbox);
				guicheckbox.setContentAreaFilled(false);
				if (jsWindow != null && strNotEmpty(jsGuiCheckedChangeMethod))
					guicheckbox.addItemListener(chckdListener = new ItemListener() {
						@Override
						public void itemStateChanged(ItemEvent e) {
							jsWindow.call(jsGuiCheckedChangeMethod, new Object[] {
									e.getStateChange() == ItemEvent.SELECTED
							});
						}
					});
				if (jsWindow != null && strNotEmpty(jsGuiEnabledChangeMethod))
					guicheckbox.addPropertyChangeListener("enabled", enbldListener = new PropertyChangeListener() {
						@Override
						public void propertyChange(PropertyChangeEvent evt) {
							jsWindow.call(jsGuiEnabledChangeMethod, new Object[] {
									evt.getNewValue()
							});
						}
					});

				int idx = getComponentIndex(ctrl);
				if (horizontalSpacing > 0 && idx > -1) {
//					add(Box.createRigidArea(new Dimension(horizontalSpacing,0)), idx + 1);
//					add(Box.createRigidArea(new Dimension(horizontalSpacing,0)), idx);
//					int w = ctrl.getWidth(), h = ctrl.getHeight();
//					debug("Resize \"Enable GUI\" from " +
//							w + "x" + h + " to " +
//							(w + 2 * horizontalSpacing) + "x" + h);
//					ctrl.setSize(w + 2 * horizontalSpacing, h);
					GridLayout gridLayout = (GridLayout)getLayout();
					gridLayout.setHgap(horizontalSpacing);
				}
			}
			else if (ctrl instanceof JButton) {
				JButton jbutt = (JButton)ctrl;
				if (hideGui)
					jbutt.setVisible(false);
				if (jbutt.getText().equalsIgnoreCase("acquire")) {
					acqbutton = jbutt;
					jbutt.setText(strNotEmpty(acquireLabel) ? acquireLabel : "Capture");
					jbutt.setToolTipText(strNotEmpty(acquireToolTip) ? acquireToolTip : "Capture Image");
					imgButton(jbutt, acquireImgUrl);
				}
				else if (jbutt.getText().equalsIgnoreCase("select")) {
					selbutton = jbutt;
					jbutt.setText(strNotEmpty(selectLabel) ? selectLabel : "Source");
					jbutt.setToolTipText(strNotEmpty(selectToolTip) ? selectToolTip : "Select Image Source");
					imgButton(jbutt, selectImgUrl);
				}
			}
	}

	private JSObject jsWindow;
	private URL documentBase;
	
	private URL imageHandlerUrl;
	private String jsCallbackMethod, jsBeforeScanCallbackMethod, jsBeforeSendCallbackMethod, jsErrorCallbackMethod,
			jsBeforeAcquireCallbackMethod;
	private String cookie;
	private boolean passJsonResult, callServerOnAcquire;

	private JCheckBox guicheckbox;
	private JButton acqbutton, selbutton;
	private ItemListener chckdListener = null;
	private PropertyChangeListener enbldListener = null;
	
	public boolean getGuiChecked() { return guicheckbox.isSelected(); }
	public void setGuiChecked(boolean chckd) {
		if (chckdListener != null)
			guicheckbox.removeItemListener(chckdListener);
		guicheckbox.setSelected(chckd);
		if (chckdListener != null)
			guicheckbox.addItemListener(chckdListener);
	}
	public boolean getGuiEnabled() { return guicheckbox.isEnabled(); }
	public void setGuiEnabled(boolean enbld) {
		if (enbldListener != null)
			guicheckbox.removePropertyChangeListener("enabled", enbldListener);
		acqbutton.setEnabled(enbld);
		selbutton.setEnabled(enbld);
		guicheckbox.setEnabled(enbld);
		if (enbldListener != null)
			guicheckbox.addPropertyChangeListener("enabled", enbldListener);
	}

//	private int exceptionCount = 0;
	
	private int prevState;
	private TwainImageInfo imageInfo;
	private int imageType;
	private int getImageType(TwainImageInfo imageInfo) {
		int imageType = BufferedImage.TYPE_3BYTE_BGR;
		switch (imageInfo.getBitsPerPixel()) {
		case  1: break;
		case  4: break;
		case  8: imageType = BufferedImage.TYPE_BYTE_GRAY; break;
		case 16: imageType = BufferedImage.TYPE_USHORT_GRAY; break;
		case 24: imageType = BufferedImage.TYPE_3BYTE_BGR; break;
		case 32: imageType = BufferedImage.TYPE_4BYTE_ABGR; break;
		}
		return imageType;
	}
	
	public void update(ScannerIOMetadata.Type type, ScannerIOMetadata scannerIOMetadata) {

		super.update(type, scannerIOMetadata);

//		if (!(scannerIOMetadata instanceof TwainIOMetadata))
//			return;
		TwainSource twainSource = ((TwainIOMetadata)scannerIOMetadata).getSource();
//		twainSource.maskTwain20Source();

//		if (!type.equals(ScannerIOMetadata.EXCEPTION))
//			exceptionCount = 0;

		if (type.equals(ScannerIOMetadata.ACQUIRED)) {
			Object image = scannerIOMetadata.getImage();
			debug("ACQUIRED: " + image);
//			safeSend(image, false);
		}
		else if (type.equals(ScannerIOMetadata.MEMORY)) {
			Info memory = ((TwainIOMetadata)scannerIOMetadata).getMemory();
			TwainBufferedImage image = (TwainBufferedImage)scannerIOMetadata.getImage();
			if (image == null || memory.getTop() == 0) {
				scannerIOMetadata.setImage(image = new TwainBufferedImage(
						imageInfo.getWidth(), imageInfo.getHeight(), imageType));
			}
			if (imageType != BufferedImage.TYPE_4BYTE_ABGR)
				System.arraycopy(memory.getBuffer(), 0,
						image.getBuffer(), memory.getTop() * memory.getBytesPerRow(),
						memory.getHeight() * memory.getBytesPerRow());
			else
				// for 32-bit convert BGRA (Windows BITMAPINFO RGBQUAD) to ABGR (Java BufferedImage)
				System.arraycopy(memory.getBuffer(), 0,
						image.getBuffer(), memory.getTop() * memory.getBytesPerRow() + 1,
						memory.getHeight() * memory.getBytesPerRow() - 1);
			debug("MEMORY: " + memory);
//			safeSend(image, false);
		}
		else if (type.equals(ScannerIOMetadata.STATECHANGE)) {
			debug("STATECHANGE: " + scannerIOMetadata.getState() + " " + scannerIOMetadata.getStateStr());
			
			int curState = scannerIOMetadata.getState();
			if (prevState == 7) { // Transferring Data
				switch (curState) {
				case 5: // Source Enabled
				case 6: // Transfer Ready
					safeSend(scannerIOMetadata.getImage(), /*curState == 5*/false); // send previous image
					break;					
				}
			}
			prevState = curState;
			if (curState < 5 && image_num > 0)
				safeSend(null, true); // send complete
			
			switch (twainSource.getState()) {
			case 6: // Transfer Ready
				safeJsCallBack(this.jsBeforeScanCallbackMethod, image_num + 1, false);
//				byte[] arrayOfByte = new byte[44];
//				try {
//					// DG_IMAGE / DAT_IMAGEINFO / MSG_GET
//					twainSource.call(2, 257, 1, arrayOfByte);
//				} catch (TwainIOException e) {
//					e.printStackTrace();
//				}
//				int width = jtwain.getINT32(arrayOfByte, 8);
//				int height = jtwain.getINT32(arrayOfByte, 12);
//				int bitsPerPixel = jtwain.getINT16(arrayOfByte, 34);
//				debug("Width x Height: " + width + " x " + height + "; Bits per pixel: " + bitsPerPixel);
				imageInfo = new TwainImageInfo(twainSource);
				try {
					imageInfo.get();
					imageType = getImageType(imageInfo);
				} catch (TwainIOException e) {
					e.printStackTrace();
				}
				debug_("" + imageInfo);
//				if (imageInfo.getBitsPerPixel() > 24)
//					twainSource.setCancel(true);
				break;
			}
		}
		else if (type.equals(ScannerIOMetadata.EXCEPTION)) {
			error("EXCEPTION: " + scannerIOMetadata.getState() + " " + scannerIOMetadata.getStateStr());
			scannerIOMetadata.getException().printStackTrace();
			error("END OF EXCEPTION MESSAGE.");
//			// prevent chained exceptions
////			if (++exceptionCount > 5) {
//			if (scannerIOMetadata.getException().getMessage().indexOf("cc=DG DAT MSG out of expected sequence") > -1) {
//				// abort transfer: end current transfer and reset transfers if there are pending
//				if (twainSource.getState() >= 6) {
//					byte[] arrayOfByte = new byte[6];
//					jtwain.setINT16(arrayOfByte, 0, -1);
//					try {
//						// DG_CONTROL / DAT_PENDINGXFERS / MSG_ENDXFER
//						twainSource.call(1, 5, 1793, arrayOfByte);
//					} catch (TwainIOException e) {
//						e.printStackTrace();
//					}
//					if (jtwain.getINT16(arrayOfByte, 0) > 0) {
//						try {
//							// DG_CONTROL / DAT_PENDINGXFERS / MSG_RESET
//							twainSource.call(1, 5, 7, arrayOfByte);
//						} catch (TwainIOException e) {
//							e.printStackTrace();
//						}
//					}
//					twainSource.setState(5);
//				}
//			}
		}
		else if (type.equals(ScannerIOMetadata.NEGOTIATE))
			debug("NEGOTIATE: " + scannerIOMetadata.getInfo());
		else if (type.equals(ScannerIOMetadata.INFO))
			debug("INFO: " + scannerIOMetadata.getInfo());
	}

	private void safeSend(Object image, boolean complete) {
		try {
			sendImage(image, complete);
		} catch (Throwable e) {
			e.printStackTrace();
			safeJsCallBack(this.jsErrorCallbackMethod, e.getMessage());
		} finally {
			if (complete)
				image_num = 0;
		}
	}
	
	private int image_num = 0;
	
	private void sendImage(Object image, boolean complete) throws IOException, ProtocolException {
//		ByteArrayOutputStream outstr = new ByteArrayOutputStream();
//		ImageIO.write((RenderedImage)image, "png", outstr);
//		byte[] buf = outstr.toByteArray();
//		buf = (URLEncoder.encode("image", "UTF-8") + "=" +
//			URLEncoder.encode(Base64.encodeToString(buf, false), "UTF-8") + "&" +
//			URLEncoder.encode("encoding", "UTF-8") + "=" +
//			URLEncoder.encode("base64", "UTF-8")).getBytes("UTF-8");
		
		int img_num = (image != null ? image_num + 1 : image_num);
		
		int w = 0, h = 0, bpp = 0, w2 = 0, h2 = 0, bpp2 = 0; 
		
		if (image != null) {
			
			Object _res = safeJsCallBack(this.jsBeforeSendCallbackMethod, img_num, complete, safeJsEval(
					"{width:"	+ (w = ((RenderedImage)image).getWidth()) +
					",height:"	+ (h = ((RenderedImage)image).getHeight()) +
					",bpp:"	+ (bpp = getImageBpp((BufferedImage)image)) +
					",type:"	+ ((BufferedImage)image).getType() +
					"}"));
		
			if (_res != null) {
				final Object width = ((JSObject)_res).getMember("width"),
						height = ((JSObject)_res).getMember("height")/*,
						up_down_both = ((JSObject)_res).getMember("scale-up-down-both")*//*,
						scaleFactor = ((JSObject)_res).getMember("scaleFactor"),
						interpolationUpscale = ((JSObject)_res).getMember("interpolationHintUpscale"),
						interpolationDownscale = ((JSObject)_res).getMember("interpolationHintDownscale")*/;
				ScaleParam sp = new ScaleParam((RenderedImage)image, width, height, /*scaleFactor*/null, /*new RenderingHints(null) {{
						put(RenderingHints.KEY_INTERPOLATION, interpolationHint((String)interpolationUpscale));
					}}*/null, /*new RenderingHints(null) {{
						put(RenderingHints.KEY_INTERPOLATION, interpolationHint((String)interpolationDownscale));
					}}*/null);
//				boolean up_only = "up".equalsIgnoreCase((String)up_down_both), down_only = "down".equalsIgnoreCase((String)up_down_both);
				if (sp.scale/* && ((up_only && sp.upscale) || (down_only && sp.downscale) || (!up_only && !down_only))*/) {
//					image = getScaledInstance((BufferedImage)image, (int)sp.w, (int)sp.h, sp.hints, sp.downscale, sp.sf);
					image = jwinbmp.resize((BufferedImage)image, (int)sp.w, (int)sp.h);
					w2 = ((RenderedImage)image).getWidth();
					h2 = ((RenderedImage)image).getHeight();
					bpp2 = getImageBpp((BufferedImage)image);
				}
			}
		}
		
		if (objEmptyString(this.imageHandlerUrl)) {
			debug("No Image Handler URL provided.");
			if (image != null) {
				//DataBuffer db = ((BufferedImage)image).getData().getDataBuffer();
				saveImageFile(image, "C:\\temp\\_twain_applet_test_" + img_num + ".png");
				int w_ = 800, h_ = 1600;
				saveImageFile(jwinbmp.resize((BufferedImage)image, w_, h_),
						"C:\\temp\\__win__resize_" + img_num + ".png");
				ScaleParam sp = new ScaleParam((RenderedImage)image, w_, h_, null, null, null);
				saveImageFile(getScaledInstance((BufferedImage)image, (int)sp.w, (int)sp.h, sp.hints, sp.downscale, sp.sf),
						"C:\\temp\\__java_resize_" + img_num + ".png");
				image_num = (complete ? 0 : img_num);
			}
			return;
		}

		//below doesn't work with HttpOnly cookies
//		JSObject jsDocument = (JSObject)this.jsWindow.getMember("document");
//		String jsCookie = (String)jsDocument.getMember("cookie");
//		debug("Cookie(from document): " + jsCookie);
//		if (strNotEmpty(jsCookie)) 
//			conn.setRequestProperty("Cookie", jsCookie);
		
		HttpPost httpPost = new HttpPost(this.imageHandlerUrl, this.cookie);
		
		for (Map.Entry<String, String> param : acquire_params) {
			httpPost.addParam(param.getKey()).writeBytes(param.getValue());
		}
		httpPost.addParam("image_num").writeBytes("" + img_num);
		if (complete) {
			httpPost.addParam("complete").writeBytes("True");
		}
		if (image != null) {
			httpPost.addParam("w").writeBytes("" + w);
			httpPost.addParam("h").writeBytes("" + h);
			httpPost.addParam("bpp").writeBytes("" + bpp);
			if (w2 > 0 && h2 > 0 && bpp2 > 0) {
				httpPost.addParam("w2").writeBytes("" + w2);
				httpPost.addParam("h2").writeBytes("" + h2);
				httpPost.addParam("bpp2").writeBytes("" + bpp2);
			}
			ImageIO.write((RenderedImage)image, "png", httpPost.addParam("image", "image.png", "image/png"));
		}
		StringBuilder response = httpPost.send(this.passJsonResult, this.passJsonResult ? new StringBuilder() : null);

		image_num = (complete ? 0 : img_num);

		if (this.passJsonResult) {
			Object res = safeJsCallBack(this.jsCallbackMethod, img_num, complete, safeJsEval(response));
			if (res != null) {
				if (res instanceof String)
					this.imageHandlerUrl = new URL(this.documentBase, (String)res);
				else if (res instanceof JSObject) {
					Object value;
					if ((value = ((JSObject)res).getMember("ImageHandler")) != null && value instanceof String)
						this.imageHandlerUrl = new URL(this.documentBase, (String)value);
					if ((value = ((JSObject)res).getMember("JSCallback")) != null && value instanceof String)
						this.jsCallbackMethod = (String)value;
					if ((value = ((JSObject)res).getMember("JSBeforeScanCallback")) != null && value instanceof String)
						this.jsBeforeScanCallbackMethod = (String)value;
					if ((value = ((JSObject)res).getMember("JSBeforeSendCallback")) != null && value instanceof String)
						this.jsBeforeSendCallbackMethod = (String)value;
					if ((value = ((JSObject)res).getMember("JSErrorCallback")) != null && value instanceof String)
						this.jsErrorCallbackMethod = (String)value;
					if ((value = ((JSObject)res).getMember("JSBeforeAcquireCallback")) != null && value instanceof String)
						this.jsBeforeAcquireCallbackMethod = (String)value;
					if ((value = ((JSObject)res).getMember("Cookie")) != null && value instanceof String)
						this.cookie = (String)value;
					if ((value = ((JSObject)res).getMember("PassJsonResult")) != null)
						this.passJsonResult = isYes(value);
					if ((value = ((JSObject)res).getMember("CallServerOnAcquire")) != null)
						this.callServerOnAcquire = isYes(value);
				}
			}
		} else
			safeJsCallBack(this.jsCallbackMethod, img_num, complete);
	}

	private Object safeJsCallBack(String callbackMethodName, Object... callbackArguments) {
		try {
			if (jsWindow != null && strNotEmpty(callbackMethodName))
				return jsWindow.call(callbackMethodName, callbackArguments);
		} catch (Throwable e) {
			e.printStackTrace();
		}
		return null;
	}

	private Object safeJsEval(Object json_string) {
		try {
			if (jsWindow != null)
				return jsWindow.eval("(function(){return " + json_string + ";})()");
		} catch (Throwable e) {
			e.printStackTrace();
		}
		return null;
	}
	
	private List<Map.Entry<String,String>> acquire_params = new ArrayList<Map.Entry<String,String>>();
	
	@Override
	public void acquire() {
		// get parameters from javascript
		Object res;
		int i = 0;
		acquire_params.clear();
		debug("Before Acquire Parameters:");
		while ((res = safeJsCallBack(this.jsBeforeAcquireCallbackMethod, i++)) != null) {
			if (((JSObject)res).getMember("value") != null) {
				String name = (String)((JSObject)res).getMember("name"),
						value = (String)((JSObject)res).getMember("value");
				debug("Name: " + name + "; Value: " + value);
				acquire_params.add(new AbstractMap.SimpleEntry<String,String>(name, value));
			}
		}
		if (this.callServerOnAcquire)
			safeSend(null, false);
		super.acquire();
	}

	private class ScaleParam {
		public final double w, h, sf;
		public final boolean scale, upscale, downscale;
		public final RenderingHints hints;
		public ScaleParam(RenderedImage image, Object width, Object height, Object scaleFactor,
				RenderingHints hintsUpscale, RenderingHints hintsDownscale) {
			if (width != null || height != null) {
				double	w_ = width != null ? ((Number)width).doubleValue() : Double.MAX_VALUE,
						h_ = height != null ? ((Number)height).doubleValue() : Double.MAX_VALUE,
						_w = image.getWidth(), _h = image.getHeight(),
						__w = _w / _h * h_;
				if (__w < w_)
					w_ = __w;
				else
					h_ = _h / _w * w_;
				if ((int)w_ != (int)_w || (int)h_ != (int)_h) {
					w = w_;
					h = h_;
					sf = scaleFactor != null ? ((Number)scaleFactor).doubleValue() : 0.6d;
					scale = true;
					upscale = (w > _w);
					downscale = !upscale;
					hints = new RenderingHints(null);
					hints.put(RenderingHints.KEY_INTERPOLATION, upscale ? RenderingHints.VALUE_INTERPOLATION_BICUBIC :  RenderingHints.VALUE_INTERPOLATION_BILINEAR);
					//hints.put(RenderingHints.KEY_RENDERING, RenderingHints.VALUE_RENDER_QUALITY);
					if (upscale) {
						if (hintsUpscale != null)
							hints.putAll(hintsUpscale);
					}
					else {
						if (hintsDownscale != null)
							hints.putAll(hintsDownscale);
					}
					return;
				}
			}
			w = h = sf = 0d;
			scale = upscale = downscale = false;
			hints = new RenderingHints(null);
		}
	}

//	private static Object interpolationHint(String interpolation) {
//		return interpolation != null && interpolation.toUpperCase().endsWith("NEAREST_NEIGHBOR") ?
//				RenderingHints.VALUE_INTERPOLATION_NEAREST_NEIGHBOR :
//		interpolation != null && ((String)interpolation).toUpperCase().endsWith("BICUBIC") ?
//				RenderingHints.VALUE_INTERPOLATION_BICUBIC :
//				RenderingHints.VALUE_INTERPOLATION_BILINEAR;
//	}

	private static BufferedImage getScaledInstance(BufferedImage img, int targetWidth, int targetHeight,
			RenderingHints hints, boolean higherQuality, final double scaleFactor) {
		int type = img.getType();
		int w, h;
		if (higherQuality) {
			// Use multi-step technique: start with original size, then
			// scale down in multiple passes with drawImage()
			// until the target size is reached
			w = img.getWidth();
			h = img.getHeight();
		} else {
			// Use one-step technique: scale directly from original
			// size to target size with a single drawImage() call
			w = targetWidth;
			h = targetHeight;
		}
		do {
			if (higherQuality && w > targetWidth) {
				w = (int)(((double)w) * scaleFactor);
				if (w < targetWidth)
					w = targetWidth;
			}
			if (higherQuality && h > targetHeight) {
				h = (int)(((double)h) * scaleFactor);
				if (h < targetHeight)
					h = targetHeight;
			}
			BufferedImage tmp = new BufferedImage(w, h, type);
			Graphics2D g2 = tmp.createGraphics();
			g2.setRenderingHints(hints);
//			g2.setRenderingHint(RenderingHints.KEY_INTERPOLATION, interpolationHint);
//			g2.setRenderingHint(RenderingHints.KEY_RENDERING, RenderingHints.VALUE_RENDER_QUALITY);
//			g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
			g2.drawImage(img, 0, 0, w, h, null);
			g2.dispose();
			img = tmp;
		} while (w != targetWidth || h != targetHeight);
		return img;
	}
	
	private void saveImageFile(Object image, final String fileName) {
		FileOutputStream stream = null;
		try {
			File file = new File(fileName);
			stream = new FileOutputStream(file);
			ImageIO.write((RenderedImage)image, "png", stream);
			debug("Created file: " + file);
		} catch (Throwable e) {
			e.printStackTrace();
		} finally {
			if (stream != null)
				try {
					stream.close();
				} catch (Throwable e) {
					e.printStackTrace();
				}
		}
	}
	
	private int getImageBpp(BufferedImage image) {
		int bpp = 0, mapSize;
		switch (image.getType()) {
		case BufferedImage.TYPE_BYTE_BINARY:
			mapSize = ((IndexColorModel)image.getColorModel()).getMapSize();
			bpp = mapSize <= 2 ? 1 : mapSize <= 4 ? 2 : 4;
			break;
		case BufferedImage.TYPE_BYTE_INDEXED:
//			mapSize = ((IndexColorModel)image.getColorModel()).getMapSize();
			bpp = 8;
			break;
		case BufferedImage.TYPE_BYTE_GRAY:
			bpp = 8;
			break;
		case BufferedImage.TYPE_USHORT_GRAY:
		case BufferedImage.TYPE_USHORT_555_RGB:
		case BufferedImage.TYPE_USHORT_565_RGB:
			bpp = 16;
			break;
		case BufferedImage.TYPE_3BYTE_BGR:
			bpp = 24;
			break;
		case BufferedImage.TYPE_4BYTE_ABGR:
			bpp = 32;
			break;
		}
		return bpp;
	}
}
