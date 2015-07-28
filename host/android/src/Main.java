package org.pileus.thesis;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.Messenger;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.webkit.WebView;
import android.widget.Toast;

public class Main extends Activity
{
	/* Private data */
	private Socket     socket;
	private Handler    handler;
	private Messenger  messenger;
	private Task       task;
	private Toast      toast;
	private boolean    running;
	private WebView    webview;

	/* Private helper methods */
	private void update(boolean running)
	{
		this.running = running;
	}

	/* Private handler methods */
	private void onRegister(Task task)
	{
		Main.debug("Main: onRegister");
		this.task = task;
		this.update(this.task.isRunning());
	}

	private void onMessage(String msg)
	{
		// Debug
		Main.debug("Main: onMessage - " + msg);
	}

	private void onNotify(String text)
	{
		Main.debug("Main: onNotify - " + text);
		this.toast.setText(text);
		this.toast.show();
	}

	/* Private service methods */
	private void register()
	{
		Main.debug("Main: register");
		startService(new Intent(this, Task.class)
				.putExtra("Command",   Task.REGISTER)
				.putExtra("Messenger", this.messenger));
	}

	private void connect()
	{
		Main.debug("Main: connect");
		startService(new Intent(this, Task.class)
				.putExtra("Command", Task.CONNECT));
		this.update(true);
	}

	private void disconnect()
	{
		Main.debug("Main: disconnect");
		startService(new Intent(this, Task.class)
				.putExtra("Command", Task.DISCONNECT));
		this.update(false);
	}

	private void quit()
	{
		stopService(new Intent(this, Task.class));
		Intent intent = new Intent(Intent.ACTION_MAIN);
		intent.addCategory(Intent.CATEGORY_HOME);
		intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
		startActivity(intent);
	}

	/* Helpers */
	public static void debug(String msg)
	{
		Log.d("Thesis", msg);
	}
	public static void debug(String msg, Exception e)
	{
		Log.d("Thesis", msg, e);
	}

	/* Activity Methods */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		try {
			super.onCreate(savedInstanceState);
			Main.debug("Main: onCreate");

			// Setup preferences
			PreferenceManager.setDefaultValues(this, R.xml.prefs, false);

			// Create socket
			this.socket    = new Socket();

			// Setup Web View
			this.webview   = new WebView(this);
			this.webview.setWebChromeClient(this.socket);
			this.webview.addJavascriptInterface(this.socket, "android");
			this.webview.getSettings().setJavaScriptEnabled(true);
			this.webview.loadUrl("file:///android_asset/index.htm");


			// Add window
			this.setContentView(this.webview);

			// Setup toast
			this.toast     = Toast.makeText(this, "", Toast.LENGTH_SHORT);

			// Setup communication
			this.handler   = new MainHandler();
			this.messenger = new Messenger(this.handler);

			// Attach to background service
			this.register();

		} catch (Exception e) {
			Main.debug("Main: onCreate - error setting content view", e);
			return;
		}
	}

	@Override
	public void onStart()
	{
		super.onStart();
		this.register();
		Main.debug("Main: onStart");
	}

	@Override
	public void onResume()
	{
		super.onResume();
		Main.debug("Main: onResume");
	}

	@Override
	public void onPause()
	{
		super.onPause();
		Main.debug("Main: onPause");
	}

	@Override
	public void onStop()
	{
		super.onStop();
		Main.debug("Main: onStop");
	}

	@Override
	public void onRestart()
	{
		super.onRestart();
		Main.debug("Main: onRestart");
	}

	@Override
	public void onDestroy()
	{
		super.onDestroy();
		Main.debug("Main: onDestroy");
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu)
	{
		MenuInflater inflater = getMenuInflater();
		inflater.inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public boolean onPrepareOptionsMenu(Menu menu)
	{
		menu.findItem(R.id.connect).setVisible(!this.running);
		menu.findItem(R.id.disconnect).setVisible(this.running);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		switch (item.getItemId()) {
			case R.id.connect:
				this.connect();
				return true;
			case R.id.disconnect:
				this.disconnect();
				return true;
			case R.id.settings:
				this.startActivity(new Intent(this, Prefs.class));
				return true;
			case R.id.quit:
				this.quit();
				return true;
			default:
				return false;
		}
	}

	/* Handler class */
	class MainHandler extends Handler
	{
		public void handleMessage(android.os.Message msg)
		{
			switch (msg.what) {
				case Task.REGISTER:
					Main.this.onRegister((Task)msg.obj);
					break;
				case Task.MESSAGE:
					Main.this.onMessage((String)msg.obj);
					break;
				case Task.NOTIFY:
					Main.this.onNotify((String)msg.obj);
					break;
				case Task.CONNECT:
					Main.this.update(true);
					break;
				case Task.DISCONNECT:
					Main.this.update(false);
					break;
				default:
					Main.debug("Main: handleMessage - unknown message '" + msg.what + "'");
					break;
			}
		}
	}
}
