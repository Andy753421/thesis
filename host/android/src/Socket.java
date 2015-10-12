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

public class Socket extends Driver implements Runnable
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
			Message    msg  = new Message.Wrap(text);
			//Main.debug("Socket: run - got packet '" + text + "'");
			this.main.broadcast(this, msg);
		} catch (Exception e) {
			Main.debug("Socket: run - error", e);
		}
	}

	public void discover() {
	}

	public void receive(Message msg) {
		//Main.debug("Socket: broadcast");

		// Check arguments
		if (this.send == null)
			return;
		if (msg == null)
			return;

		// Broadcast in background
		new AsyncTask<Message, Void, Void>() {
			protected Void doInBackground(Message... args) {
				try {
					Message    msg  = args[0];
					String     txt  = msg.write();
					CharBuffer cbuf = CharBuffer.wrap(txt);
					ByteBuffer bbuf = encoder.encode(cbuf);
					Socket.this.send.send(bbuf, SEND_ADDR);
					//Main.debug("Socket: broadcast - -> "
					//	+ SEND_ADDR + " : " + text);
				} catch (Exception e) {
					Main.debug("Socket: broadcast - error sending message: " + e);
				}
				return null;
			}
		}.execute(msg);
	}
}
