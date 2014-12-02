package com.fb.screenshotapp;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.lang.reflect.Method;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.Socket;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.util.Enumeration;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Matrix;
import android.graphics.Point;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.Bitmap.Config;
import android.os.IBinder;
import android.util.Log;
import android.view.Display;
import android.view.Surface;
import android.view.WindowManager;


public class ScreenCapService extends Service  
{
	private String TAG = "APP";

	private String pUsername="";
	private String serverUrl="";
	private int serverPort=8888;

	private boolean isConnectServerBoolean = false;
	private boolean isSendScreen = false;	

	private boolean isStartService = false;

	@Override  
	public void onCreate()  
	{  
		Log.i(TAG, "Service onCreate--->");          
		super.onCreate();  

		pUsername = getLocalIpAddress();
		isStartService = true;
	}

	@Override  
	public void onStart(Intent intent, int startId)  
	{ 	
		Log.i(TAG,"Screen capture service start.");
		if(isSendScreen){
			Log.i(TAG,"Screen capture service has already started.");
			return ;
		}

		SharedPreferences preferences=getSharedPreferences("configs", Context.MODE_PRIVATE);
		serverUrl = preferences.getString("IP", "");
		serverPort = preferences.getInt("port", 8888);
		Log.i(TAG, "serverUrl: "+serverUrl); 
		Log.i(TAG, "serverPort: "+serverPort); 

		StartConnect();

		MainThread mt = new MainThread();
		mt.start();		
	}  

	@Override  
	public void onDestroy()  
	{  
		Log.i(TAG,"----------------------------------------------------"); 
		Log.i(TAG,"SendScreenService stopped."); 	

		StopConnect();
		isStartService = false;
		isConnectServerBoolean = false;
		isSendScreen = false;
		super.onDestroy();  
	}  

	@Override 
	public IBinder onBind(Intent intent)  
	{  
		return null;  
	}  


	public void StartConnect(){
		if(!isConnectServerBoolean){
			Log.i(TAG,"Start connecting."); 
			isConnectServerBoolean = true;
			Thread th = new SendCommandThread("PHONECONNECT|"+pUsername+"|");
			th.start(); 
		}
	}

	public void StopConnect(){
		if(isConnectServerBoolean){
			Log.i(TAG,"Stop connecting."); 
			isConnectServerBoolean = false;
			Thread th = new SendCommandThread("PHONEDISCONNECT|"+pUsername+"|");
			th.start(); 	
		}
	}

	class SendCommandThread extends Thread{
		private String command;
		public SendCommandThread(String command){
			this.command=command;
		}

		public void run(){
			Log.i(TAG,"Send command thread started."); 
			try {
				Socket socket=new Socket(serverUrl,serverPort);
				PrintWriter out = new PrintWriter(socket.getOutputStream());
				out.println(command);
				out.flush();
				Log.i(TAG,"Send command thread stop."); 
				isSendScreen = true;

				out.close();
				socket.close();
				out = null;
				socket = null;
			} catch (UnknownHostException e) {
				e.printStackTrace();
			} catch (IOException e) {
				e.printStackTrace();
			}  
		}
	}

	class MainThread extends Thread{
		private byte byteBuffer[] = new byte[1024];
		private OutputStream outsocket = null;	
		private int i =0;

		public void run(){
			Log.i(TAG,"Send data to PC thread created."); 
			
			while(isStartService){
				if(isConnectServerBoolean){
					if(isSendScreen){
						try {
							long totalStartTime = System.currentTimeMillis(); 
							
							byte[] bt = writeImageOutputStream();
							long startTime=System.currentTimeMillis();  

							Socket tempSocket = null;
							try {
								tempSocket = new Socket(serverUrl, serverPort);
							} catch (UnknownHostException e) {
								e.printStackTrace();
							} catch (IOException e) {
								e.printStackTrace();
							}
							outsocket = tempSocket.getOutputStream();

							String msg=java.net.URLEncoder.encode("PHONESCREEN|"+pUsername+"|","utf-8");
							byte[] buffer= msg.getBytes();
							outsocket.write(buffer);

							ByteArrayInputStream inputstream = new ByteArrayInputStream(bt);
							int amount;
							while ((amount = inputstream.read(byteBuffer)) != -1) {
								outsocket.write(byteBuffer, 0, amount);
							}
							tempSocket.close(); 
							long endTime=System.currentTimeMillis(); 
							Log.i(TAG,"Wifi send raw data £º "+(endTime-startTime)+"ms"); 

							bt = null;
							inputstream = null;
							outsocket = null;
							System.gc();
							Log.i(TAG,"Total time £º "+(endTime-totalStartTime)+"ms"); 
							Log.i(TAG,"*********************************************************"); 

						} catch (IOException e) {
							Log.i(TAG,"IOException occured."); 
						}

					}
				}
			}
		}
	}

	public String getLocalIpAddress() {  
		try {  
			for (Enumeration<NetworkInterface> en = NetworkInterface  
					.getNetworkInterfaces(); en.hasMoreElements();) {  
				NetworkInterface intf = en.nextElement();  
				for (Enumeration<InetAddress> enumIpAddr = intf  
						.getInetAddresses(); enumIpAddr.hasMoreElements();) {  
					InetAddress inetAddress = enumIpAddr.nextElement();  
					if (!inetAddress.isLoopbackAddress() && !inetAddress.isLinkLocalAddress()) {  
						return inetAddress.getHostAddress().toString();  
					}  
				}  
			}  
		} catch (SocketException ex) {  
			Log.e("WifiPreference IpAddress", ex.toString());  
		}  
		return null;  
	}

	public static byte[] getBytesFromInputStream(InputStream is)
	{
		ByteArrayOutputStream os = new ByteArrayOutputStream();

		byte[] buffer = new byte[0xFFFF];

		try {
			for (int len; (len = is.read(buffer)) != -1;)
				os.write(buffer, 0, len);
		} catch (IOException e) {
			e.printStackTrace();
		}

		try {
			os.flush();
		} catch (IOException e) {
			e.printStackTrace();
		}

		return os.toByteArray();

	}


	class Screenshot {
		public Buffer pixels;
		public int width;
		public int height;
		public int bpp;

		public boolean isValid() {
			if (pixels == null || pixels.capacity() == 0 || pixels.limit() == 0) return false;
			if (width <= 0 || height <= 0)	return false;
			return true;
		}
	}


	@SuppressWarnings("deprecation")
	private int getScreenRotation()  {
		WindowManager wm = (WindowManager)getSystemService(WINDOW_SERVICE);
		Display disp = wm.getDefaultDisplay();

		// check whether we operate under Android 2.2 or later
		try {
			Class<?> displayClass = disp.getClass();
			Method getRotation = displayClass.getMethod("getRotation");
			int rot = ((Integer)getRotation.invoke(disp)).intValue();

			switch (rot) {
			case Surface.ROTATION_0:	return 0;
			case Surface.ROTATION_90:	return 90;
			case Surface.ROTATION_180:	return 180;
			case Surface.ROTATION_270:	return 270;
			default:					return 0;
			}
		} catch (NoSuchMethodException e) {
			int orientation = disp.getOrientation();
			if(orientation==Configuration.ORIENTATION_UNDEFINED){

				Configuration config = getResources().getConfiguration();
				orientation = config.orientation;

				if(orientation==Configuration.ORIENTATION_UNDEFINED){
					if(disp.getWidth()==disp.getHeight()){
						orientation = Configuration.ORIENTATION_SQUARE;
					}else{ 
						if(disp.getWidth() < disp.getHeight()){
							orientation = Configuration.ORIENTATION_PORTRAIT;
						}else{ 
							orientation = Configuration.ORIENTATION_LANDSCAPE;
						}
					}
				}
			}

			return orientation == 1 ? 0 : 90; // 1 for portrait, 2 for landscape
		} catch (Exception e) {
			return 0; // bad, I know ;P
		}
	}

	private Screenshot retreiveRawScreenshot() throws Exception {
		try {
			WindowManager wm = (WindowManager)getSystemService(WINDOW_SERVICE);
			Display disp = wm.getDefaultDisplay();

			Screenshot ss = new Screenshot();
			Point xysize = new Point();
			disp.getSize(xysize);	
			ss.width = xysize.x/2;
			ss.height = xysize.y/2;
//			ss.width = xysize.x;
//			ss.height = xysize.y;
			ss.bpp = 32;

			long startTime=System.currentTimeMillis();  
			ByteBuffer bytes = ByteBuffer.wrap(fbjni.screenPngBytes(ss.width*ss.height*ss.bpp/8));
			ss.pixels = bytes;
			long endTime=System.currentTimeMillis(); 
			Log.i(TAG,"Get raw data ---raw data£º "+(endTime-startTime)+"ms"); 

			return ss;
		}
		catch (Exception e) {
			throw new Exception(e);
		}
	}

	private  byte[] writeImageOutputStream() {
		int ImageQuality = 90;
		Screenshot ss = null;
		try {
			ss = retreiveRawScreenshot();
		} catch (Exception e) {
			e.printStackTrace();
		}

		if (ss == null || !ss.isValid())	
			throw new IllegalArgumentException();

		// resolve screenshot's BPP to actual bitmap pixel format
		Bitmap.Config pf;
		switch (ss.bpp) {
		case 16:	pf = Config.RGB_565; break;
		case 32:	pf = Config.ARGB_8888; break;
		default:	pf = Config.ARGB_8888; break;
		} 

		// create appropriate bitmap and fill it wit data
		long startTime=System.currentTimeMillis();
		Bitmap bmp = null;
		try {
			bmp = Bitmap.createBitmap(ss.width, ss.height, pf);
		} catch (OutOfMemoryError e) {
			Log.i(TAG,"Bitmap---Out of memory!");  
		}
		bmp.copyPixelsFromBuffer(ss.pixels);

		Log.i(TAG,"Create Bitmap---------------------------!");
		ss.pixels = null;
		ss = null;
		System.gc();

		// handle the screen rotation
		int rot = getScreenRotation();
		if (rot != 0) {
			Matrix matrix = new Matrix();
			matrix.postRotate(-rot);
			bmp = Bitmap.createBitmap(bmp, 0, 0, bmp.getWidth(), bmp.getHeight(), matrix, true);
		}
		
		Log.i(TAG,"Rotate Bitmap---------------------------!");
		ByteArrayOutputStream outStream = new ByteArrayOutputStream(); 
		bmp.compress(CompressFormat.PNG, ImageQuality, outStream);
		long endTime=System.currentTimeMillis();  
		Log.i(TAG,"pNG Bitmap cost time---raw data£º "+(endTime-startTime)+"ms");  

		try {
			outStream.close();
		} catch (IOException e) {
			e.printStackTrace();
		}

		if(bmp != null && !bmp.isRecycled()){
			bmp.recycle();
			bmp = null;
			System.gc();
		}
		return outStream.toByteArray();
	}

}

