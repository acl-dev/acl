package com.iker.gid;
import java.io.IOException;
import java.io.OutputStream;
import java.net.HttpURLConnection;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.xml.sax.SAXException;

/**
 * 按XML格式获得GID数据，若要使用此方式取得唯一ID，则服务器
 * 必须是启用了HTTP协议访问方式
 * @author zsx
 *
 */
public final class GidClientXml extends GidClient {

	public GidClientXml(String ip, int port, String tag)
	{
		super(ip, port, tag);
	}
	
	@Override
	public long gidNext() {
		// TODO Auto-generated method stub
		HttpURLConnection urlConnection = null;
		try {
			urlConnection = OpenUrl("xml"); // 连接服务器
			OutputStream os = urlConnection.getOutputStream();
			String body = "<request cmd='new_gid' tag='" + tagName + "' />"; 
			os.write(body.getBytes());
			os.flush();
			//os.close();
			
			/* 返回的数据格式: <respond status='ok|error' gid=xxx tag='xxx' msg='xxx' err='xxx' /> */

			try {
				DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
				DocumentBuilder db = dbf.newDocumentBuilder();
				Document document = db.parse(urlConnection.getInputStream());
				Element root = document.getDocumentElement();
				
				String status = root.getAttribute("status");
				String gidStr = root.getAttribute("gid");
				if (status.equals("ok") == false || gidStr.equals("")) {
					String err = root.getAttribute("err");
					if (err.equals("") == false)
						errnum = Integer.parseInt(err);
					return -1;
				}
				long gid = Long.parseLong(gidStr);
				return gid;
			} catch (ParserConfigurationException e) {
				e.printStackTrace();
			} catch(SAXException e) {
				e.printStackTrace();
			}
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			if (urlConnection != null)
				urlConnection.disconnect();
		}
		return -1;
	}
	
	public static void main(String[] args) {
		final String ip = "192.168.1.251";
		final int port = 7072;
		final String tag = "default";
		
		GidClient gidClient = new GidClientXml(ip, port, tag);
		for (int i = 0; i < 100; i++) {
			long gid = gidClient.gidNext();
			System.out.print(">>xml gid: " + gid + "\r\n");
		}
	}
}
