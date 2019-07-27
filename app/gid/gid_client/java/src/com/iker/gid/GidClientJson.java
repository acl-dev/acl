package com.iker.gid;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;

import net.sf.json.JSONObject;

/**
 * 鎸� JSON 鏁版嵁鏍煎紡鑾峰緱GID锛岃嫢瑕佷娇鐢ㄦ鏂瑰紡鍙栧緱鍞竴ID锛屽垯鏈嶅姟鍣�
 * 蹇呴』鏄惎鐢ㄤ簡HTTP鍗忚璁块棶鏂瑰紡
 * @author zsx
 *
 */
public final class GidClientJson extends GidClient {

	public GidClientJson(String ip, int port, String tag)
	{
		super(ip, port, tag);
	}
	
	@Override
	public long gidNext() {
		// TODO Auto-generated method stub
		HttpURLConnection urlConnection = null;
		try {
			urlConnection = OpenUrl("json");
			OutputStream os = urlConnection.getOutputStream();
			
			/* 璇锋眰鐨勬暟鎹牸寮忥細{ cmd: 'new_gid'; tag: 'xxx'; } */
			JSONObject json = new JSONObject();
			json.put("cmd", "new_gid");
			json.put("tag", tagName);
 
			os.write(json.toString().getBytes());
			os.flush();
			//os.close();
			
			/* 杩斿洖鐨勬暟鎹牸寮�: { status: 'ok|error'; gid: xxx; tag: 'xxx'; msg: 'xxx'; err: 'xxx'; } */
			InputStream in = urlConnection.getInputStream();
			BufferedReader reader = new BufferedReader(new InputStreamReader(in));
			String buf="";
			char[] tmp = new char[128];
			while (true) {
				
				if (reader.read(tmp) == -1)
					break;
				buf += new String(tmp);
			}
			buf.toLowerCase(); // 鍏堣浆涓哄皬鍐�

			JSONObject jsonRet = JSONObject.fromObject(buf);
			String status = jsonRet.getString("status");
			if (status.equals("ok") == false) {
				String err = jsonRet.getString("err");
				if (err.equals("") == false)
					errnum = Integer.parseInt(err);
				return -1;
			}
			long gid = jsonRet.getLong("gid");
			return gid;
		} catch (IOException e) {
			e.printStackTrace();
		} catch (Exception e) {
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
		
		GidClient gidClient = new GidClientJson(ip, port, tag);
		for (int i = 0; i < 100; i++) {
			long gid = gidClient.gidNext();
			System.out.print(">>json gid: " + gid + "\r\n");
		}
	}
}
