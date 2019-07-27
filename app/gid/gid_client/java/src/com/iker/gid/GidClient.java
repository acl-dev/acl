package com.iker.gid;

import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.InetSocketAddress;
import java.net.MalformedURLException;
import java.net.Socket;
import java.net.SocketAddress;
import java.net.URL;
import java.net.UnknownHostException;

/**
 * 鎶借薄鎺ュ彛
 * @author zsx
 *
 */
public abstract class GidClient {
	protected int errnum = 0;
	protected String serverIp = "192.168.1.251";
	protected int serverPort = 7072;
	protected String tagName = "default";
	protected int connTimeout = 10000; /* 杩炴帴瓒呮椂鏃堕棿涓�10绉� */
	protected int rdTimeout = 30000; /* 璇昏秴鏃舵椂闂翠负30绉� */
	
	public GidClient(String ip, int port, String tag)
	{
		if (ip.isEmpty() == false)
			this.serverIp = ip;
		if (port > 0)
			this.serverPort = port;
		if (tag != "")
			this.tagName = tag;
	}
	
	/**
	 * 鑾峰緱涓嬩竴涓敮涓€ID鍙�
	 * @return 杩斿洖鍞竴鐨�64浣嶆暣鏁帮紝濡傛灉杩斿洖鍊� < 0 鍒欒〃绀哄嚭閿�
	 */
	abstract public long gidNext();

	protected HttpURLConnection OpenUrl(String gidFmt)
	{
		String urlAddr = "http://" + serverIp + ":" + serverPort + "/";
		HttpURLConnection urlConnection = null;
		
		try {
			URL url = new URL(urlAddr);
			urlConnection = (HttpURLConnection) url.openConnection();
			urlConnection.setConnectTimeout(connTimeout);
			urlConnection.setReadTimeout(rdTimeout);
			
			// 璁剧疆鏄惁浠巋ttpUrlConnection璇诲叆锛岄粯璁ゆ儏鍐典笅鏄痶rue;
			urlConnection.setDoInput(true);
			
			// 璁剧疆鏄惁鍚慼ttpUrlConnection杈撳嚭锛屽洜涓鸿繖涓槸post璇锋眰锛�
			// 鍙傛暟瑕佹斁鍦� http姝ｆ枃鍐咃紝鍥犳闇€瑕佽涓簍rue, 榛樿鎯呭喌涓嬫槸false;
			urlConnection.setDoOutput(true);
		
			// 璁剧疆涓� POST 璇锋眰鏂瑰紡
			urlConnection.setRequestMethod("POST");
			
			// Post 璇锋眰涓嶈兘浣跨敤缂撳瓨
			urlConnection.setUseCaches(false);
			urlConnection.setInstanceFollowRedirects(false);
			urlConnection.setRequestProperty("User-Agent",
					"ESite JAVA Agent");
			urlConnection.setRequestProperty("Content-Type",
					"text/plain;charset=utf8");
			urlConnection.setRequestProperty("x-gid-format", gidFmt);
		} catch (MalformedURLException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
		
		return (urlConnection);
	}
	
	/**
	 * 璁剧疆IO璇昏秴鏃舵椂闂�
	 * @param timeout 瓒呮椂鏃堕棿锛堝崟浣嶄负绉掞級锛屽鏋滀笉璁剧疆姝ゆ椂闂达紝
	 *  鍒欏唴閮ㄧ己鐪佸€间负30绉�
	 */
	public void setRdTimeout(int timeout)
	{
		// 灏嗙杞负姣
		rdTimeout = timeout * 1000;
	}
	
	/**
	 * 璁剧疆缁滆繛鎺ユ椂闂�
	 * @param timeout 瓒呮椂鏃堕棿锛堝崟浣嶄负绉掞級锛屽鏋滀笉璁剧疆姝ゆ椂闂达紝
	 *  鍒欏唴閮ㄧ己鐪佸€间负10绉�
	 */
	public void setConnectTimeout(int timeout)
	{
		// 灏嗙杞负姣
		connTimeout = timeout * 1000;
	}
	
	protected Socket OpenTcp() {
		Socket socket = null;
		try {
			socket = new Socket();
			socket.setSoTimeout(rdTimeout);
			SocketAddress socketAddress =
					new InetSocketAddress(serverIp, serverPort);
			// 甯﹁繛鎺ヨ秴鏃舵柟寮忚繛鎺ユ湇鍔″櫒
			socket.connect(socketAddress, connTimeout);
		} catch (UnknownHostException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
		return socket;
	}

	/**
	 * 杩斿洖鍑洪敊鍙�
	 * @return 閿欒鍙�
	 */
	int getLastError() {
		return errnum;
	}
}
