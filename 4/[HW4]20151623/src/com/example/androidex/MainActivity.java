package com.example.androidex;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.SystemClock;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.util.Random;

public class MainActivity extends Activity implements Button.OnClickListener{
	
	LinearLayout linear;
	EditText data;
	Button btn;
	OnClickListener ltn;
	TextView timerTextView;
	
	int row, col, wid, hei;
	int dx[] = {1, -1, 0, 0};
	int dy[] = {0, 0, -1, 1};
	int[] puzzleStats;
	boolean flag;
	
	Thread timer;
	TimerService ms;
	boolean isService = false;
	
	// service connection object
	ServiceConnection conn = new ServiceConnection() {
		@Override
		public void onServiceConnected(ComponentName name, IBinder service) {
			TimerService.TimerBinder mb = (TimerService.TimerBinder) service;
			ms = mb.getService();
			isService = true;
		}
		
		@Override
		public void onServiceDisconnected(ComponentName name) {
			isService = false;
		}
	};
	
	private void shuffleIntArray(int[] a) {
		int len = a.length;
		Random r = new Random();
		for (int i = 0; i < len; i++) {
			int idx = r.nextInt(len);
			int t = a[i];
			a[i] = a[idx];
			a[idx] = t;
		}
		
		if (isSolved()) {
			int idx = r.nextInt(len-1);
			int t = a[len-1];
			a[len-1] = a[idx];
			a[idx] = t;
		}
	}
	
	@Override
	public void onWindowFocusChanged(boolean hasFocus) {
		// store dimension info
		wid = linear.getWidth();
		hei = linear.getHeight() - 150;
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		linear = (LinearLayout)findViewById(R.id.container);
		
		data=(EditText)findViewById(R.id.editText1);
		Button btn=(Button)findViewById(R.id.button1);
		timerTextView=(TextView)findViewById(R.id.textView1);
		
		flag = false;
		
		ltn=new OnClickListener(){
			public void onClick(View v){
				if (flag) {
					clearPuzzle();
					flag = false;
					timerTextView.setText("");
					if(isService) {
						unbindService(conn);
						isService = false;
					}
				}
				
				String[] inputString=data.getText().toString().split("\\s");
				// only [row] [column] input matches
				if (inputString.length != 2)
					return;
				
				// get row & column number from user
				try {
					row = Integer.parseInt(inputString[0]);
					col = Integer.parseInt(inputString[1]);
				} catch (NumberFormatException e) {
					return;
				}

				data.getText().clear();
				
				// 0 < row, column < 6
				if (5 < row || row <= 0 || 5 < col || col <= 0)
					return;
				
				// do not support 1x1 puzzle
				if (row + col == 2)
					return;
				
				// initialize puzzle status
				puzzleStats = new int[row*col];
				for (int i = 0; i < puzzleStats.length; i++) {
					puzzleStats[i] = i;
				}
				// shuffle
				shuffleIntArray(puzzleStats);	
				
				SystemClock.sleep(100);
				
				drawPuzzle();
				flag = true;
				Intent intent = new Intent(
						MainActivity.this,
						TimerService.class
						);
				bindService(intent, conn, Context.BIND_AUTO_CREATE);
				Thread timer = new Thread(new PuzzleTimer());
				timer.start();
			}
		};
		btn.setOnClickListener(ltn);
	}
	
	@Override
	public void onClick(View view) {
		Button pressed = (Button)view;
		int val = Integer.parseInt((String)pressed.getText());
		int idx = 0;
	
		for (int i = 0; i < row * col; i++)
			if (puzzleStats[i] == val)
				idx = i;
		
		for (int i = 0; i < 4; i++) {
			int cX = idx % col + dx[i];
			int cY = idx / col + dy[i];
			int curr = cX + cY*col;
			
			if (cX < 0 || cX >= col || cY < 0 || cY >= row)
				continue;
			
			if (puzzleStats[curr] == 0) {
				puzzleStats[curr] = val;
				puzzleStats[idx] = 0;
				clearPuzzle();
				drawPuzzle();
				break;
			}
		}
		
		if (isSolved()) {
			clearPuzzle();
			flag = false;
			if(isService) {
				unbindService(conn);
				isService = false;
			}
		}
	}
	
	public void drawPuzzle() {
		System.out.println(row + " " + col + " " + wid + " " + hei);
		
		for (int i = 0; i < row; i++) {
			// make sub-layout for single row
			LinearLayout rowLinear = new LinearLayout(this);
			rowLinear.setLayoutParams(new LinearLayout.LayoutParams(wid, hei / row));
			rowLinear.setTag(i + "th row container");
			
			// make #(col) buttons
			for (int j = 0; j < col; j++) {
				Button piece = new Button(this);
				piece.setLayoutParams(new LinearLayout.LayoutParams(wid / col, hei / row));
				piece.setText(String.valueOf(puzzleStats[i * col + j]));
				
				if (puzzleStats[i * col + j] == 0)
					piece.setBackgroundColor(Color.BLACK);
				else
					piece.setOnClickListener(this);
				
				// link to sub-layout
				rowLinear.addView(piece);
			}
			
			// link to parent layout
			linear.addView(rowLinear);
		}
	}
	
	public void clearPuzzle() {
		// clear all
		for (int i = 0; i < row; i++) {
			LinearLayout curr = (LinearLayout)linear.getChildAt(3);
			curr.removeAllViewsInLayout();
			linear.removeView(curr);
		}
	}
	
	public boolean isSolved() {
		for (int i = 0; i < row * col - 1; i++)
			if (puzzleStats[i] != i+1)
				return false;
		return true;
	}
	
	private class PuzzleTimer implements Runnable {
		private Handler handler = new Handler();
		
		@Override
		public void run() {
			while (flag) {
				if (ms == null)
					continue;
				
				handler.post(new Runnable() {
					@Override
					public void run() {
						int elapsed = ms.getElapsed();
						int min = elapsed / 10 / 60;
						int sec = elapsed / 10 % 60;
						int msec = elapsed % 10;
						
						timerTextView.setText(min+":"+sec+"."+msec);
					}
				});
				
				try {
					Thread.sleep(100);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
			}
		}
	}
}
