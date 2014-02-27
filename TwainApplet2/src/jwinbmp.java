import java.awt.image.BufferedImage;

import uk.co.mmscomputing.util.JarLib;


public class jwinbmp {

	public static native BufferedImage resize(BufferedImage img, int width, int height);

	static{
		// win : load library 'jwinbmp.dll'
		JarLib.load(jwinbmp.class, "jwinbmp");  
	}
}
