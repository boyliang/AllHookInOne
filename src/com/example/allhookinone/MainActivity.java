package com.example.allhookinone;

import java.lang.reflect.Method;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;

public class MainActivity extends Activity {
	
	static final class TestClass{
		private static String getStringStatic(int a, String b){
			return String.format("%d %s from getStringStatic", a, b);
		}
	}
	
	static class MyEntity{
		public String value = "HelloWorld";
	}

	@Override
	public Context getApplicationContext() {
		Log.i("TTT", "abstract getApplicationContext");
		return super.getApplicationContext();
	}
	
	private String getString(int a, String b){
		return  String.format("%d %s from getString", a, b);
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
		
		//抽象方法测试
		Method method;
		try {
			Log.i("TTT", "AbstractMethod Test Before: " + getApplicationContext());
			method = MainActivity.class.getDeclaredMethod("getApplicationContext");
			HookUtils.hookMethod(method, "()Landroid/content/Context;");
			Log.i("TTT", "AbstractMethod Test After: " + getApplicationContext());
		} catch (NoSuchMethodException e) {
			e.printStackTrace();
		}
		
		// 静态方法测试
		try {
			Log.i("TTT", "before0: " + TestClass.getStringStatic(1, "2"));
			method = TestClass.class.getDeclaredMethod("getStringStatic", int.class, String.class);
			HookUtils.hookMethod(method, "(ILjava/lang/String;)Ljava/lang/String;");
			Log.i("TTT", "after0: " + TestClass.getStringStatic(1, "2"));
		} catch (NoSuchMethodException e) {
			e.printStackTrace();
		}
		
		// 原始类型与系统类型混合测试
		try {
			Log.i("TTT", "before1: " + getString(1, "aa"));
			method = MainActivity.class.getDeclaredMethod("getString", int.class, String.class);
			HookUtils.hookMethod(method, "(ILjava/lang/String;)Ljava/lang/String;");
			Log.i("TTT", "after1: " + getString(1, "aa"));
		} catch (NoSuchMethodException e) {
			e.printStackTrace();
		}
		
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
		
		// ELFHook测试
		HookUtils.elfhook();
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}
	
	

}
