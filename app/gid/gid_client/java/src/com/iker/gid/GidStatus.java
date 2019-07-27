package com.iker.gid;

/**
 * @author zsx
 * 璇ョ被涓昏瀹氫箟浜咷ID鐨勯敊璇彿浠ュ強灏嗛敊璇彿杞负瀛楃涓叉弿杩颁俊鎭殑鍐呭
 */
public final class GidStatus {
	public static final int gidOk = 200;
	public static final int gidErrInit = 400;
	public static final int gidErrConn = 401;
	public static final int gidErrIo = 402;
	public static final int gidErrProto = 403;
	public static final int gidErrServer = 404;
	public static final int gidErrSid = 500;
	public static final int gidErrOverride = 501;
	public static final int gidErrSave = 502;
	
	/**
	 * 灏嗛敊璇彿杞负瀛楃涓叉弿杩颁俊鎭�
	 * @param gidStatus
	 * @return String
	 */
	public static String toString(int gidStatus)
	{
		switch (gidStatus)
		{
		case gidOk:
			return "ok";
		case gidErrInit:
			return "gid_client_init should called first";
		case gidErrConn:
			return "connect server error";
		case gidErrIo:
			return "readwrite from server error";
		case gidErrProto:
			return "gid protocol error";
		case gidErrServer:
			return "gid server internal error";
		case gidErrSid:
			return "sid invalid";
		case gidErrOverride:
			return "gid override";
		case gidErrSave:
			return "gid save error";
		default:
			return "unknown error number";
		}
	}
}
