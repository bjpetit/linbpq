package net.g8bpq.bpqtermtcp;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.Bundle;
import android.text.Editable;
import android.text.method.KeyListener;
import android.view.KeyEvent;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.ScrollView;
import android.widget.TextView;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.View;
import android.view.Menu;
import android.view.MenuItem;
import android.util.TypedValue;

import net.g8bpq.bpqtermtcp.R;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.Socket;
import java.net.UnknownHostException;

public class MainActivity extends AppCompatActivity {

    Socket s;
    OutputStream out;
    BufferedReader input;
    TextView Output;
    EditText Input;
    ScrollView SV;
    String Host = "localhost";
    int Port = 8011;
    String User = "";
    String Pass = "";

    String OutBuffer = "";				// The main screen buffer

    public final static String EXTRA_MESSAGE = "com.example.myapp.MESSAGE";
    private static boolean NeedScroll = false;

    @Override
    public void onBackPressed()
    {
        return;
    }
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)  {
        if (keyCode == KeyEvent.KEYCODE_BACK && event.getRepeatCount() == 0)
        {
            //	Check for Close

            AlertDialog.Builder dlgAlert  = new AlertDialog.Builder(this);

            dlgAlert.setMessage("Do you want to close BPQTermTCP?");
            dlgAlert.setTitle("Confirm Close");
            dlgAlert.setPositiveButton("Yes",
                    new DialogInterface.OnClickListener() {
                        public void onClick(DialogInterface dialog, int which) {
                            finish();
                        }
                    });
            dlgAlert.setNegativeButton("No", null);
            dlgAlert.setCancelable(true);
            dlgAlert.create().show();

            return true;
        }
        return super.onKeyDown(keyCode, event);
    }
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        SharedPreferences settings = getSharedPreferences("BPQTermTCP", 0);

        Toolbar myToolbar = (Toolbar) findViewById(R.id.my_toolbar);
        setSupportActionBar(myToolbar);

        android.os.StrictMode.ThreadPolicy policy = new android.os.StrictMode.ThreadPolicy.Builder().permitAll().build();
        android.os.StrictMode.setThreadPolicy(policy);

        int currentOrientation = getResources().getConfiguration().orientation;
        if (currentOrientation == Configuration.ORIENTATION_LANDSCAPE)
        {
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        }
        else
        {
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        }

        Output = (TextView) findViewById(R.id.Output);
        Input = (EditText) findViewById(R.id.Input);
        SV = (ScrollView) findViewById(R.id.Scroll);

        ConfigActivity.TextSize = settings.getInt("TextSize", 10);
        ConfigActivity.Hide = settings.getBoolean("Hide", true);

        Output.setTextSize(TypedValue.COMPLEX_UNIT_SP, ConfigActivity.TextSize);
        Input.setTextSize(TypedValue.COMPLEX_UNIT_SP, ConfigActivity.TextSize);

        Input.setOnEditorActionListener(new EditText.OnEditorActionListener()
        {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == 4 || actionId == 0)
                {
                    sendMessage();
                    return true;
                }
                else {
                    return false;
                }
            }
        });

        Input.setKeyListener(new KeyListener()
        {
            @Override
            public int getInputType() {
                return 1;
            }


            @Override

            public boolean onKeyDown(View view, Editable text, int keyCode,
                                     KeyEvent event) {
                return false;
            }

            @Override
            public boolean onKeyOther(View view, Editable text, KeyEvent event) {
                return false;
            }

            @Override
            public boolean onKeyUp(View view, Editable text, int keyCode, KeyEvent event)
            {
                if (keyCode == 66)
                {
                    // Return key

                    sendMessage();
                }
                return false;
            }


            @Override
            public void clearMetaKeyState(View view, Editable content, int states)
            {
            }
        });
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.activity_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        //check selected menu item

        if(item.getItemId() == R.id.exit){
            this.finish();
            return true;
        }

        if(item.getItemId() == R.id.dis){
            try
            {
                s.close();
            }
            catch (Exception e)
            {
                e.printStackTrace();
                Output.setText(e.getMessage());
            }
            Output.setText("Disonnecting");
            NeedScroll = true;
            return true;
        }

        if(item.getItemId() == R.id.settings)
        {
            Intent intent = new Intent(this, ConfigActivity.class);
            startActivity(intent);
        }

        if ((item.getItemId() == R.id.con1) || (item.getItemId() == R.id.con2))
        {
            // Connect button pressed
            // Restore settings

            SharedPreferences settings = getSharedPreferences("BPQTermTCP", 0);

            if (item.getItemId() == R.id.con1)
            {
                Host = settings.getString("Host", "localhost");
                Port = settings.getInt("Port", 8011);
                User = settings.getString("User", "");
                Pass = settings.getString("Pass", "");
            }
            else
            {
                Host = settings.getString("Host2", "localhost");
                Port = settings.getInt("Port2", 8011);
                User = settings.getString("User2", "");
                Pass = settings.getString("Pass2", "");
            }

            Output.setText("Connecting to " + Host);
            NeedScroll = true;

            try
            {
                s = new Socket(Host, Port);

                //outgoing stream redirect to socket

                out = s.getOutputStream();
                input = new BufferedReader(new InputStreamReader(s.getInputStream()));

                String signon = User + "\r" + Pass + "\rBPQTermTCP\r";

                byte[] bytes = signon.getBytes("UTF-8");
                out.write(bytes);

            }
 /*           catch (UnknownHostException e)
            {
                // TODO Auto-generated catch block

                Output.setText("Resolve Failed " + e.getMessage());
                e.printStackTrace();
            }
   */
            catch (Exception e)
            {
                e.printStackTrace();
                Output.setText(e.getMessage());
            }
          /*  catch (SecurityException e)
            {
                e.printStackTrace();
                Output.setText(e.getMessage());
            }
            catch (IOException e)
            {
                e.printStackTrace();
                Output.setText(e.getMessage());
            }
          */
            finally
            {
                if (s == null)
                {
                    return true;
                }
            }

            // Start thread to read from socket

            new Thread(new Runnable()
            {
                public void run()
                {
                    ReadFromSocket();			// this runs until the socket closes
                }
            }
            ).start();

            return true;
        }

        // other buttons

        return false;
    }

    public void sendMessage()
    {
        if (s == null)
        {
            Output.setText("Not Connected");
            NeedScroll = true;
            return;
        }
        try
        {
            String message = Input.getText().toString() + "\r";

            OutBuffer = OutBuffer + message + "\r\n";
            Output.setText(OutBuffer);
            Output.scrollBy(0, 10000);

            byte[] bytes = message.getBytes("UTF-8");
            out.write(bytes);
        }

        catch (UnknownHostException e) {Output.setText(e.getMessage()); return;}
        catch (IOException e) {Output.setText(e.getMessage()); return;}

        Input.setText("");

        Output.setTextSize(TypedValue.COMPLEX_UNIT_SP, ConfigActivity.TextSize);
        Input.setTextSize(TypedValue.COMPLEX_UNIT_SP, ConfigActivity.TextSize);

        if (ConfigActivity.Hide)
        {
            ((InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE)).toggleSoftInput(0, 0);
        }
    }

    public void ReadFromSocket()
    {
        int count;
        char[] inchars;

        while(true)
        {
            try
            {
                Thread.sleep(1000);
            }
            catch (InterruptedException e) {}

            if (NeedScroll)
            {
                NeedScroll = false;

                runOnUiThread(new Runnable()

                              {
                                  public void run()
                                  {
                                      SV.post(new Runnable() {
                                          public void run() {
                                              SV.fullScroll(ScrollView.FOCUS_DOWN);
                                          }
                                      });
                                  }
                              }
                );
            }

            try
            {
                if (input.ready())
                {
                    inchars = new char[10000];
                    count = input.read(inchars, 0, 10000);

                    char lastchar = inchars[count - 1];

                    String st = new String(inchars, 0, count);
                    String[] lines = st.split("\r");

                    int i = 0;

                    while(i < lines.length)
                    {
                        if (i < lines.length - 1 || lastchar == 13)		// last line
                        {
                            OutBuffer = OutBuffer + lines[i] + "\r\n";
                        }
                        else
                        {
                            OutBuffer = OutBuffer + lines[i];
                        }
                        i = i + 1;
                    }

                    // Can't update UI from this thread

                    runOnUiThread(new Runnable()
                                  {
                                      public void run()
                                      {
                                          Output.setText(OutBuffer);
                                          NeedScroll = true;
                                      }
                                  }
                    );
                }
            }
            catch (IOException e)
            {
                Output.setText(e.getMessage());

                // Need to indicate disconnected

                return;				// close thread
            }
        }
    }




    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    // Used to load the 'native-lib' library on application startup.
static {
      System.loadLibrary("native-lib");
   }
}
