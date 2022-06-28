package com.iker.gid;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.Socket;
import java.util.StringTokenizer;
import java.util.Vector;

/**
 * 命令行方式从GID服务器取得唯一ID号，是否应该使用该方式取得唯一ID号，
 * 应该取决于服务器的配置选项
 * @author zsx
 *
 */
public final class GidClientCmdLine extends GidClient {
	public GidClientCmdLine(String ip, int port, String tag)
	{
		super(ip, port, tag);
	}
	
	/**
	 * name-value 对类型类
	 */
	public final class NameValue {
		private String name;
		private String value;
		
		public NameValue(String name, String value) {
			this.name = name;
			this.value = value;
		}
		
		String getName()
		{
			return name;
		}
		
		String getValue()
		{
			return value;
		}
	}

	/**
	 * 拆分字符串的类，该类将“名值”对进行拆分，数据格式为：
	 * name1^value1|name2^value2|...
	 */
	public final class Tokens {
		private static final String spliter1 = "|";
		private static final String spliter2 = "\\^";
		
		private Vector<NameValue> tokens = new Vector<NameValue>();;
		
		public Tokens(String content) {
			StringTokenizer sk = new StringTokenizer(content, spliter1);
			while (sk.hasMoreTokens()) {
				String str = sk.nextToken();
				String[] nv = str.split(spliter2);
				if (nv.length != 2)
					continue;
				tokens.add(new NameValue(nv[0], nv[1]));
			}
		}
	
		public String getString(String name)
		{
			for (int i = 0; i < tokens.size(); i++) {
				NameValue token = tokens.get(i);
				if (token.getName().equalsIgnoreCase(name))
					return token.getValue();
			}
			
			return "";
		}
		
		public long getLong(String name)
		{
			for (int i = 0; i < tokens.size(); i++) {
				NameValue token = tokens.get(i);
				if (token.getName().equalsIgnoreCase(name))
					return Long.parseLong(token.getValue());
			}
			
			return -1;
		}
		
		public int getInt(String name)
		{
			for (int i = 0; i < tokens.size(); i++) {
				NameValue token = tokens.get(i);
				if (token.getName().equalsIgnoreCase(name))
					return Integer.parseInt(token.getValue());
			}
			
			return -1;
		}
	}
		
	@Override
	public long gidNext() {
		Socket socket = null;
		try {
			socket = OpenTcp();
			
			OutputStream out = socket.getOutputStream();
			/* 请求的数据格式：CMD^new_gid|TAG^default */
			String buf = "CMD^new_gid|TAG^" + tagName + "\r\n";
			out.write(buf.getBytes());
			out.flush();
			//out.close();
			
			/* 响应数据格式：status^ok[|error]|gid^xxx[|tag^xxx|err^xxx|msg^xxx] */
			InputStream in = socket.getInputStream();
			BufferedReader reader = new BufferedReader(new InputStreamReader(in));
			buf = reader.readLine();
			
			out.close();
			in.close();
			
			Tokens tokens = new Tokens(buf);
			String status = tokens.getString("status");
			if (status.equalsIgnoreCase("ok") == false) {
				errnum = tokens.getInt("err");
				return -1;
			}
			return tokens.getLong("gid");
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			try {
				if (socket != null)
					socket.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		return -1;
	}
	
	public static void main(String[] args) {
		final String ip = "192.168.1.251";
		final int port = 7072;
		final String tag = "default";
		
		GidClient gidClient = new GidClientCmdLine(ip, port, tag);
		
		for (int i = 0; i < 100; i++) {
			long gid = gidClient.gidNext();
			System.out.print(">>cmdline gid: " + gid + "\r");
		}
	}
}
