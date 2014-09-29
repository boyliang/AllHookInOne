package com.example.allhookinone;

import java.lang.reflect.Method;

import android.os.Bundle;
import android.app.Activity;
import android.util.Log;
import android.view.Menu;

public class MainActivity extends Activity {
	
	static class MyEntity{
		public String value = "HelloWorld";
	}

	private String getString(int a, String b){
		return a + "str: " + b;
	}
	
	private String getString2(String... str1){
		return str1[0] + " " + str1[1];
	}
	
	private String getString3(MyEntity entity){
		return entity.value;
	}
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		// 原始类型与系统类型混合测试
		Log.i("TTT", "before1: " + getString(0, "a"));
		Method method;
		try {
			method = MainActivity.class.getDeclaredMethod("getString", int.class, String.class);
			HookUtils.hookMethod(method, "(ILjava/lang/String;)Ljava/lang/String;");
		} catch (NoSuchMethodException e) {
			e.printStackTrace();
		}
		Log.i("TTT", "after1: " + getString(1, "a"));
		
		// 可变类型测试
		Log.i("TTT", "before2: " + getString2("aa", "bb"));
		try {
			method = MainActivity.class.getDeclaredMethod("getString2", String[].class);
			HookUtils.hookMethod(method, "([Ljava/lang/String;)Ljava/lang/String;");
		} catch (NoSuchMethodException e) {
			e.printStackTrace();
		}
		Log.i("TTT", "after2: " + getString2("aa", "bb"));
		
		
		// 自定义类型测试
		MyEntity entity = new MyEntity();
		Log.i("TTT", "before3: " + getString3(entity));
		try {
			method = MainActivity.class.getDeclaredMethod("getString3", MyEntity.class);
			HookUtils.hookMethod(method, "(Lcom/example/allhookinone/MainActivity$MyEntity;)Ljava/lang/String;");
		} catch (NoSuchMethodException e) {
			e.printStackTrace();
		}
		Log.i("TTT", "after3: " + getString3(entity));
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}
	
	

}
