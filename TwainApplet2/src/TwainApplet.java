import java.awt.BorderLayout;
import java.awt.Color;
import javax.swing.JApplet;

import uk.co.mmscomputing.device.scanner.Scanner;
import uk.co.mmscomputing.device.twain.TwainIOException;

@SuppressWarnings("serial")
public class TwainApplet extends JApplet implements Runnable {

	public void init() {
	    //Execute a job on the event-dispatching thread:
	    //creating this applet's GUI.
	    try {
	        javax.swing.SwingUtilities.invokeAndWait(this);
	    } catch (Exception e) {
	        System.err.println("Failed to create GUI");
	        e.printStackTrace();
	    }
	    getContentPane().setBackground(Color.WHITE);
	}

	public void run() {
		try {
			getContentPane().add(tp = new TwainPanelUpload(Scanner.getDevice(), this), BorderLayout.CENTER);
		} catch (TwainIOException e) {
			e.printStackTrace();
		}
	}

	TwainPanelUpload tp;
	
	public void acquire() { tp.acquire(); }
	public void select() { tp.select(); }
	
	public boolean getGuiChecked() { return tp.getGuiChecked(); }
	public void setGuiChecked(boolean chckd) { tp.setGuiChecked(chckd); }
	public boolean getGuiEnabled() { return tp.getGuiEnabled(); }
	public void setGuiEnabled(boolean enbld) { tp.setGuiEnabled(enbld); }
}
