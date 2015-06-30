package org.pileus.thesis;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.Handler;
import android.os.Messenger;
import android.preference.PreferenceManager;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.format.DateFormat;
import android.text.style.BackgroundColorSpan;
import android.text.style.ForegroundColorSpan;
import android.text.style.StrikethroughSpan;
import android.text.style.StyleSpan;
import android.text.style.UnderlineSpan;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TabHost;
import android.widget.TabWidget;
import android.widget.TextView;
import android.widget.Toast;

public class Main extends Activity
{
	/* Private data */
	private Handler      handler;
	private Messenger    messenger;
	private Task         task;
	private Toast        toast;
	private boolean      running;

	/* Private helper methods */
	private void update(boolean running)
	{
		this.running = running;
	}

	/* Private handler methods */
	private void onRegister(Task task)
	{
		Log.d("Thesis", "Main: onRegister");
		this.task = task;
		this.update(this.task.isRunning());
	}

	private void onMessage(String msg)
	{
		// Debug
		Log.d("Thesis", "Main: onMessage - " + msg);
	}

	private void onNotify(String text)
	{
		Log.d("Thesis", "Main: onNotify - " + text);
		this.toast.setText(text);
		this.toast.show();
	}

	/* Private service methods */
	private void register()
	{
		Log.d("Thesis", "Main: register");
		startService(new Intent(this, Task.class)
				.putExtra("Command",   Task.REGISTER)
				.putExtra("Messenger", this.messenger));
	}

	private void connect()
	{
		Log.d("Thesis", "Main: connect");
		startService(new Intent(this, Task.class)
				.putExtra("Command", Task.CONNECT));
		this.update(true);
	}

	private void disconnect()
	{
		Log.d("Thesis", "Main: disconnect");
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

	/* Activity Methods */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		try {
			super.onCreate(savedInstanceState);
			Log.d("Thesis", "Main: onCreate");

			// Setup preferences
			PreferenceManager.setDefaultValues(this, R.xml.prefs, false);

			// Setup main layout
			this.setContentView(R.layout.main);

			// Setup toast
			this.toast     = Toast.makeText(this, "", Toast.LENGTH_SHORT);

			// Setup communication
			this.handler   = new MainHandler();
			this.messenger = new Messenger(this.handler);

			// Attach to background service
			this.register();

		} catch (Exception e) {
			Log.d("Thesis", "Error setting content view", e);
			return;
		}
	}

	@Override
	public void onStart()
	{
		super.onStart();
		this.register();
		Log.d("Thesis", "Main: onStart");
	}

	@Override
	public void onResume()
	{
		super.onResume();
		Log.d("Thesis", "Main: onResume");
	}

	@Override
	public void onPause()
	{
		super.onPause();
		Log.d("Thesis", "Main: onPause");
	}

	@Override
	public void onStop()
	{
		super.onStop();
		Log.d("Thesis", "Main: onStop");
	}

	@Override
	public void onRestart()
	{
		super.onRestart();
		Log.d("Thesis", "Main: onRestart");
	}

	@Override
	public void onDestroy()
	{
		super.onDestroy();
		Log.d("Thesis", "Main: onDestroy");
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
					Log.d("Thesis", "Main: unknown message - " + msg.what);
					break;
			}
		}
	}
}
