package org.pileus.thesis;

import java.util.UUID;

import android.util.Log;
import android.webkit.ConsoleMessage;
import android.webkit.JavascriptInterface;
import android.webkit.WebChromeClient;
import android.webkit.WebView;

public class Socket extends WebChromeClient
{
	// WebChromeClient
	public boolean onConsoleMessage(ConsoleMessage cm) {
		Log.d("Thesis", "WebView: "
			+ cm.sourceId() + ":"
			+ cm.lineNumber());
		Log.d("Thesis", "WebView: "
			+ cm.message());
		return true;
	}

	// Javscript Interface
	@JavascriptInterface
	public boolean connect() {
		Log.d("Thesis", "WebView: Connect");
		return true;
	}

	@JavascriptInterface
	public String listen(Object discover, Object receive) {
		Log.d("Thesis", "WebView: Listen: " + discover + ", " + receive);
		return UUID.randomUUID().toString();
	}

	@JavascriptInterface
	public void remove(String handle) {
		Log.d("Thesis", "WebView: Remove: " + handle);
	}

	@JavascriptInterface
	public void broadcast(Object mesg, String handle) {
		Log.d("Thesis", "WebView: Broadcast: " + handle);
	}
}
