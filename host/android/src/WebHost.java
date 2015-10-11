package org.pileus.thesis;

import android.webkit.WebView;

import android.webkit.ConsoleMessage;
import android.webkit.JavascriptInterface;
import android.webkit.WebChromeClient;
import android.webkit.WebView;

class WebHost extends Driver {

	private WebView   webview;
	private JsHandler handler;

	// Constructor
	public WebHost(Main main) {
		// Web View
		this.main    = main;
		this.webview = new WebView(main);
		this.handler = new JsHandler();

		// Setup Web View
		this.webview.setWebChromeClient(this.handler);
		this.webview.addJavascriptInterface(this.handler, "android");
		this.webview.getSettings().setJavaScriptEnabled(true);
		this.webview.loadUrl("file:///android_asset/index.htm");

		// Add window
		main.setContentView(this.webview);
	}

	// Discover
	public void discover() {
	}

	// Receive
	public void receive(Message msg)
	{
		String str  = msg.write();
		String call = "android_callback(\"" +
			str.replaceAll("\\\\", "\\\\\\\\")
			   .replaceAll("\"",   "\\\\\"")
		+ "\")";
		JsRunner run = new JsRunner(this.webview, call);
		this.main.runOnUiThread(run);
				
	}

	// Post class
	class JsRunner implements Runnable {
		public WebView view;
		public String  code;
		public JsRunner(WebView view, String code) {
			this.view = view;
			this.code = code;
		}
		public void run() {
			this.view.loadUrl("javascript: " + code);
		}
	}

	// Chrome Client
	class JsHandler extends WebChromeClient {
		@JavascriptInterface
		public void broadcast(String mesg) {
		}

		public boolean onConsoleMessage(ConsoleMessage cm) {
			int    max = 50;
			String msg = cm.message();
			if (msg.length() > max)
				msg = msg.substring(0,max);
			//Main.debug("WebHost: onConsoleMessage - "
			//	+ cm.sourceId() + ":"
			//	+ cm.lineNumber());
			Main.debug("WebHost: onConsoleMessage - " + msg);
			return true;
		}
	}
}
