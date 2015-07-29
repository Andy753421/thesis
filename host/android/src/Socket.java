package org.pileus.thesis;

import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.channels.DatagramChannel;
import java.nio.charset.Charset;
import java.nio.charset.CharsetDecoder;
import java.nio.charset.CharsetEncoder;

import android.content.Context;
import android.os.AsyncTask;
import android.webkit.ConsoleMessage;
import android.webkit.JavascriptInterface;
import android.webkit.WebChromeClient;
import android.webkit.WebView;

public class Socket extends WebChromeClient implements Runnable
{
	// UDP Data
	private final SocketAddress SEND_ADDR =
		new InetSocketAddress("255.255.255.255", 12345);

	private final SocketAddress RECV_ADDR =
		new InetSocketAddress("0.0.0.0", 12345);

	private Charset         charset = Charset.forName("UTF-8");
	private CharsetEncoder  encoder = charset.newEncoder();
	private CharsetDecoder  decoder = charset.newDecoder();

	private DatagramChannel send;
	private DatagramChannel recv;

	private Main            main;
	private Thread          thread;

	// Main
	public Socket(Main main) {
		Main.debug("Socket: setting up network");
		try {
			this.main = main;

			this.send = DatagramChannel.open();
			this.send.socket().setBroadcast(true);

			this.recv = DatagramChannel.open();
			this.recv.socket().setReuseAddress(true);
			this.recv.socket().bind(this.RECV_ADDR);

			this.thread = new Thread(this);
			this.thread.start();
		} catch (Exception e) {
			Main.debug("Socket: error creating socket: "
					+ e + ", " + this.RECV_ADDR);
		}
	}

	// WebChromeClient
	public boolean onConsoleMessage(ConsoleMessage cm) {
		int    max = 50;
		String msg = cm.message();
		if (msg.length() > max)
			msg = msg.substring(0,max);
		//Main.debug("Socket: onConsoleMessage - "
		//	+ cm.sourceId() + ":"
		//	+ cm.lineNumber());
		Main.debug("Socket: onConsoleMessage - " + msg);
		return true;
	}

	// Thread interface
	@Override
	public void run() {
		Main.debug("Socket: run - starting");
		ByteBuffer bbuf = ByteBuffer.allocate(0xFFFF);
		while (true) try {
			bbuf.clear();
			SocketAddress addr = this.recv.receive(bbuf);
			if (addr == null) {
				Main.debug("Socket: run - skip");
				continue;
			}
			bbuf.flip();
			CharBuffer cbuf = decoder.decode(bbuf);
			String     text = cbuf.toString();
			//Main.debug("Socket: run - got packet '" + text + "'");
			this.main.eval(text);
		} catch (Exception e) {
			Main.debug("Socket: run - error", e);
		}
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
