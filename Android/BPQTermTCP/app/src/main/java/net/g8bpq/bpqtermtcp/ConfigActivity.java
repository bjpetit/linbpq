package net.g8bpq.bpqtermtcp;

/**
 * Created by John on 14/12/2016.
 */


        import android.app.Activity;

        import android.content.SharedPreferences;
        import android.os.Bundle;
        import android.support.v7.app.AppCompatActivity;
        import android.view.View;
        import android.widget.CheckBox;
        import android.widget.EditText;

        import net.g8bpq.bpqtermtcp.R;


public class ConfigActivity extends AppCompatActivity
{
    EditText inputHost;
    EditText inputPort;
    EditText inputUser;
    EditText inputPass;
    EditText inputHost2;
    EditText inputPort2;
    EditText inputUser2;
    EditText inputPass2;
    EditText inputSize;
    CheckBox inputHide;

    String Host = "";
    int Port = 0;
    String User = "";
    String Pass = "";

    String Host2 = "";
    int Port2 = 0;
    String User2 = "";
    String Pass2 = "";

    static Boolean Hide = true;
    static int TextSize = 10;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.config);

        inputHost = (EditText) findViewById(R.id.Host);
        inputPort = (EditText) findViewById(R.id.Port);
        inputUser = (EditText) findViewById(R.id.User);
        inputPass = (EditText) findViewById(R.id.Pass);
        inputHost2 = (EditText) findViewById(R.id.Host2);
        inputPort2 = (EditText) findViewById(R.id.Port2);
        inputUser2 = (EditText) findViewById(R.id.User2);
        inputPass2 = (EditText) findViewById(R.id.Pass2);
        inputSize = (EditText) findViewById(R.id.Size);
        inputHide = (CheckBox) findViewById(R.id.checkBox1);

        SharedPreferences settings = getSharedPreferences("BPQTermTCP", 0);
        Host = settings.getString("Host", "localhost");
        Port = settings.getInt("Port", 8011);
        User = settings.getString("User", "");
        Pass = settings.getString("Pass", "");
        Host2 = settings.getString("Host2", "localhost");
        Port2 = settings.getInt("Port2", 8011);
        User2 = settings.getString("User2", "");
        Pass2 = settings.getString("Pass2", "");
        TextSize = settings.getInt("TextSize", 10);
        Hide = settings.getBoolean("Hide", true);

        inputHost.setText(Host);
        inputPort.setText(String.valueOf(Port));
        inputUser.setText(User);
        inputPass.setText(Pass);

        inputHost2.setText(Host2);
        inputPort2.setText(String.valueOf(Port2));
        inputUser2.setText(User2);
        inputPass2.setText(Pass2);

        inputSize.setText(String.valueOf(TextSize));
        inputHide.setChecked(Hide);
    }

    public void Save(View view)
    {
        Host = inputHost.getText().toString();
        Port = Integer.parseInt(inputPort.getText().toString());
        User = inputUser.getText().toString();
        Pass = inputPass.getText().toString();

        Host2 = inputHost2.getText().toString();
        Port2 = Integer.parseInt(inputPort2.getText().toString());
        User2 = inputUser2.getText().toString();
        Pass2 = inputPass2.getText().toString();

        Hide = inputHide.isChecked();
        TextSize = Integer.parseInt(inputSize.getText().toString());

        // We need an Editor object to make preference changes.

        SharedPreferences setsettings = getSharedPreferences("BPQTermTCP", 0);
        SharedPreferences.Editor editor = setsettings.edit();

        editor.putString("Host", Host);
        editor.putString("User", User);
        editor.putString("Pass", Pass);
        editor.putInt("Port", Port);

        editor.putString("Host2", Host2);
        editor.putString("User2", User2);
        editor.putString("Pass2", Pass2);
        editor.putInt("Port2", Port2);

        editor.putInt("TextSize", TextSize);
        editor.putBoolean("Hide", Hide);

        // Commit the edits!

        editor.commit();

        finish();

    }
}


