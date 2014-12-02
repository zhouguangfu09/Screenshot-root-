package com.fb.screenshotapp;

public class fbjni {
    static {
        System.loadLibrary("fbbuffer");
    }
	
	static public native String test();
    static public native byte[] screenPngBytes(int size);
    static public native int bufferlength();

}
