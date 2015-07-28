package org.pileus.thesis;

import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.channels.DatagramChannel;
import java.nio.charset.Charset;
import java.nio.charset.CharsetDecoder;
import java.nio.charset.CharsetEncoder;

import android.os.AsyncTask;
import android.util.Log;
import android.webkit.ConsoleMessage;
import android.webkit.JavascriptInterface;
import android.webkit.WebChromeClient;
import android.webkit.WebView;

public class Socket extends WebChromeClient
{
	// UDP Data
	private final SocketAddress SEND_ADDR =
		new InetSocketAddress("255.255.255.255", 12345);

	private final SocketAddress RECV_ADDR =
		new InetSocketAddress("127.0.0.1", 12345);

	private Charset         charset = Charset.forName("UTF-8");
	private CharsetEncoder  encoder = charset.newEncoder();
	private CharsetDecoder  decoder = charset.newDecoder();

	private DatagramChannel send;
	private DatagramChannel recv;

	// Main
	public Socket() {
		Main.debug("Socket: setting up network");
		try {
			this.send = DatagramChannel.open();
			this.send.socket().setBroadcast(true);

			this.recv = DatagramChannel.open();
			this.recv.socket().setReuseAddress(true);
			this.recv.socket().bind(this.RECV_ADDR);
		} catch (Exception e) {
			Main.debug("Socket: error creating socket: "
					+ e + ", " + this.RECV_ADDR);
		}
	}

	// WebChromeClient
	public boolean onConsoleMessage(ConsoleMessage cm) {
		Main.debug("Socket: onConsoleMessage - "
			+ cm.sourceId() + ":"
			+ cm.lineNumber());
		Main.debug("Socket: onConsoleMessage - "
			+ cm.message());
		return true;
	}

	// Socket interface
	public void callback() {
		// socket receive
	}

	// Javscript Interface
	@JavascriptInterface
	public void broadcast(String mesg) {
		//Main.debug("Socket: broadcast");

		// Check arguments
		if (this.send == null)
			return;
		if (mesg == null)
			return;

		// Broadcast in background
		new AsyncTask<String, Void, Void>() {
			protected Void doInBackground(String... args) {
				try {
					String     text = args[0];
					CharBuffer cbuf = CharBuffer.wrap(text);
					ByteBuffer bbuf = encoder.encode(cbuf);
					Socket.this.send.send(bbuf, SEND_ADDR);
					//Main.debug("Socket: broadcast - -> "
					//	+ SEND_ADDR + " : " + text);
				} catch (Exception e) {
					Main.debug("Socket: broadcast - error sending message: " + e);
				}
				return null;
			}
		}.execute(mesg);
	}
}
