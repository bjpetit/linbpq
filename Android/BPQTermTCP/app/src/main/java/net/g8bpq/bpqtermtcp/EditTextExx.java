package net.g8bpq.bpqtermtcp;

/**
 * Created by John on 14/12/2016.
 */


import android.content.Context;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.widget.EditText;


public class EditTextExx extends EditText {

    public EditTextExx(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            switch (keyCode) {
                case KeyEvent.KEYCODE_BACK:
                    break;
                case KeyEvent.KEYCODE_MENU:
                    break;
                case 66:
          //          MainActivity.sendMessage();
                    return true;
                default:
                    break;
            }
        }
        return false;
    }

}