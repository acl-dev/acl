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
 * 抽象接口
 * @author zsx
 *
 */
public abstract class GidClient {
	protected int errnum = 0;
	protected String serverIp = "192.168.1.251";
	protected int serverPort = 7072;
	protected String tagName = "default";
	protected int connTimeout = 10000; /* 连接超时时间为10秒 */
	protected int rdTimeout = 30000; /* 读超时时间为30秒 */
	
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
	 * 获得下一个唯一ID号
	 * @return 返回唯一的64位整数，如果返回值 < 0 则表示出错
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
			
			// 设置是否从httpUrlConnection读入，默认情况下是true;
			urlConnection.setDoInput(true);
			
			// 设置是否向httpUrlConnection输出，因为这个是post请求，
			// 参数要放在 http正文内，因此需要设为true, 默认情况下是false;
			urlConnection.setDoOutput(true);
		
			// 设置为 POST 请求方式
			urlConnection.setRequestMethod("POST");
			
			// Post 请求不能使用缓存
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
	 * 设置IO读超时时间
	 * @param timeout 超时时间（单位为秒），如果不设置此时间，
	 *  则内部缺省值为30秒
	 */
	public void setRdTimeout(int timeout)
	{
		// 将秒转为毫秒
		rdTimeout = timeout * 1000;
	}
	
	/**
	 * 设置络连接时间
	 * @param timeout 超时时间（单位为秒），如果不设置此时间，
	 *  则内部缺省值为10秒
	 */
	public void setConnectTimeout(int timeout)
	{
		// 将秒转为毫秒
		connTimeout = timeout * 1000;
	}
	
	protected Socket OpenTcp() {
		Socket socket = null;
		try {
			socket = new Socket();
			socket.setSoTimeout(rdTimeout);
			SocketAddress socketAddress =
					new InetSocketAddress(serverIp, serverPort);
			// 带连接超时方式连接服务器
			socket.connect(socketAddress, connTimeout);
		} catch (UnknownHostException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
		return socket;
	}

	/**
	 * 返回出错号
	 * @return 错误号
	 */
	int getLastError() {
		return errnum;
	}
}
