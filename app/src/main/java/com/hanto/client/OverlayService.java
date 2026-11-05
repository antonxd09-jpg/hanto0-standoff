package com.hanto.client;
import android.app.Service;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.os.IBinder;
import android.view.Gravity;
import android.view.WindowManager;
import android.webkit.JavascriptInterface;
import android.webkit.WebView;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

public class OverlayService extends Service {
    private WindowManager windowManager;
    private WebView webView;
    @Override
    public void onCreate() {
        super.onCreate();
        windowManager = (WindowManager) getSystemService(WINDOW_SERVICE);
        webView = new WebView(this);
        WindowManager.LayoutParams params = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.MATCH_PARENT, WindowManager.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL,
                PixelFormat.TRANSLUCENT);
        params.gravity = Gravity.TOP | Gravity.LEFT;
        webView.setBackgroundColor(0x00000000);
        webView.getSettings().setJavaScriptEnabled(true);
        webView.addJavascriptInterface(new WebBridge(), "Android");
        webView.loadUrl("file:///android_asset/index.html");
        windowManager.addView(webView, params);
    }
    @Override
    public IBinder onBind(Intent intent) { return null; }
    @Override
    public void onDestroy() {
        super.onDestroy();
        if (webView != null) windowManager.removeView(webView);
    }
    public class WebBridge {
        @JavascriptInterface
        public void setFeature(String feature, boolean state) {
            try {
                DatagramSocket socket = new DatagramSocket();
                String msg = feature + ":" + (state ? "1" : "0");
                byte[] buf = msg.getBytes();
                InetAddress address = InetAddress.getByName("127.0.0.1");
                DatagramPacket packet = new DatagramPacket(buf, buf.length, address, 8888);
                socket.send(packet);
                socket.close();
            } catch (Exception e) {}
        }
    }
}
