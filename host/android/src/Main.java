package org.pileus.thesis;

import java.util.ArrayList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.webkit.WebView;
import android.widget.Toast;

public class Main extends Activity
{
	/* Private data */
	private WebView      webview;
	private Socket       socket;
	private Toast        toast;
	private Timer        timer;
	private TimerTask    task;
	private List<Driver> drivers;

	/* Debug Helpers */
	public static void debug(String msg)
	{
		Log.d("Thesis", msg);
	}
	public static void debug(String msg, Exception e)
	{
		Log.d("Thesis", msg, e);
	}
	public void notice(String msg)
	{
		this.toast.setText(msg);
		this.toast.show();
	}

	/* Driver functions */
	public void discover() {
		for (Driver drv : this.drivers)
			drv.discover();
	}

	public void receive(String code)
	{
		final String url = "javascript: android_callback(\"" +
			code
			.replaceAll("\\\\", "\\\\\\\\")
			.replaceAll("\"",   "\\\\\"")
		+ "\")";
		this.runOnUiThread(new Runnable() {
			public void run() {
				Main.this.webview.loadUrl(url);
		        }
		});
	}

	public void broadcast(String msg) {
		Main.debug("broadcast: " + msg);
		this.socket.broadcast(msg);
	}

	public void load(Driver drv) {
		drv.init(this);
		this.drivers.add(drv);
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

			// Create Helpers
			this.socket    = new Socket(this);
			this.webview   = new WebView(this);
			this.drivers   = new ArrayList<Driver>();

			// Load drivers
			this.load(new DataStore());

			// Setup Web View
			this.webview.setWebChromeClient(this.socket);
			this.webview.addJavascriptInterface(this.socket, "android");
			this.webview.getSettings().setJavaScriptEnabled(true);
			this.webview.loadUrl("file:///android_asset/index.htm");

			// Add window
			this.setContentView(this.webview);

			// Setup toast
			this.toast     = Toast.makeText(this, "", Toast.LENGTH_SHORT);

		} catch (Exception e) {
			Main.debug("Main: onCreate - error setting content view", e);
			return;
		}
	}

	@Override
	public void onStart()
	{
		super.onStart();
		//this.register();
		Main.debug("Main: onStart");
	}

	@Override
	public void onResume()
	{
		super.onResume();
		Main.debug("Main: onResume");

		/* Start timer */
		Log.d("Control", "Starting timer");
		this.task = new TimerTask() {
			public void run() {
				Main.this.runOnUiThread(new Runnable() {
					public void run() {
						Main.this.discover();
					}
				});
			}
		};
		this.timer = new Timer();
		this.timer.scheduleAtFixedRate(this.task, 0, 1000/1);
	}

	@Override
	public void onPause()
	{
		super.onPause();
		Main.debug("Main: onPause");

		/* Stop timer */
		this.timer.cancel();
		this.timer = null;
		this.task  = null;
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
	public boolean onOptionsItemSelected(MenuItem item)
	{
		switch (item.getItemId()) {
			case R.id.settings:
				this.startActivity(new Intent(this, Prefs.class));
				return true;
			default:
				return false;
		}
	}
}
