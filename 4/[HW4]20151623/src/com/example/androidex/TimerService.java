package com.example.androidex;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;

public class TimerService extends Service {
		private boolean solved;
		private int elapsed;
		
		IBinder mBinder = new TimerBinder();
		
		class TimerBinder extends Binder {
			TimerService getService() {
				return TimerService.this;
			}
		}
		
		@Override
		public IBinder onBind(Intent intent) {
			System.out.println("onBind");
			return mBinder;
		}
		
		@Override
		public void onCreate() {
			super.onCreate();
			System.out.println("onCreate");

			solved = false;
			elapsed = 0;
			Thread timer = new Thread(new PuzzleTimer());
			timer.start();
		}
		
		@Override
		public void onDestroy() {
			super.onDestroy();
			System.out.println("onDestroy");

			solved = true;
		}
		
		public int getElapsed() {
			return elapsed;
		}
		
		class PuzzleTimer implements Runnable {
			@Override
			public void run() {
				elapsed = 0;
				
				while (!solved) {
					try {
						Thread.sleep(100);
					} catch (InterruptedException e) {
						e.printStackTrace();
					}
					elapsed += 1;
				}
			}
		}
	}