package org.pileus.thesis;

import android.app.Service;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.IBinder;
import android.os.Messenger;
import android.preference.PreferenceManager;
import android.util.Log;

public class Task extends Service
{
	/* Commands */
	public static final int REGISTER   = 0;
	public static final int MESSAGE    = 1;
	public static final int NOTIFY     = 2;
	public static final int CONNECT    = 3;
	public static final int DISCONNECT = 4;

	/* Private data */
	private SharedPreferences prefs;
	private Messenger         messenger;
	private Thread            thread;

	/* Private methods */
	private void handle(int cmd, Messenger mgr)
	{
		// Validate messenger
		if (cmd != REGISTER && mgr != null && mgr != this.messenger) {
			Log.d("Thesis", "Task: handle - invalid messenger");
		}

		// Setup communication with Main
		if (cmd == REGISTER) {
			Log.d("Thesis", "Task: handle - register");
		}

		// Create client thread
		if (cmd == CONNECT && this.thread == null) {
			Log.d("Thesis", "Task: handle - connect");
		}

		// Stop client thread
		if (cmd == DISCONNECT && this.thread != null) {
			Log.d("Thesis", "Task: handle - register");
		}
	}

	/* Public methods */
	public boolean isRunning()
	{
		return this.thread != null;
	}

	/* Service Methods */
	@Override
	public void onCreate()
	{
		Log.d("Thesis", "Task: onCreate");
		super.onCreate();

		this.prefs = PreferenceManager.getDefaultSharedPreferences(this);
	}

	@Override
	public void onDestroy()
	{
		Log.d("Thesis", "Task: onDestroy");
		this.handle(DISCONNECT, null);
	}

	@Override
	public int onStartCommand(Intent intent, int flags, int startId) {
		Log.d("Thesis", "Task: onStart");
		super.onStartCommand(intent, flags, startId);
		int       cmd = intent.getExtras().getInt("Command");
		Messenger mgr = (Messenger)intent.getExtras().get("Messenger");
		this.handle(cmd, mgr);
		return START_STICKY;
	}

	@Override
	public IBinder onBind(Intent intent)
	{
		Log.d("Thesis", "Task: onBind");
		return null;
	}
}
