package njust.pr.opticalmatrixcalculator;


import android.annotation.SuppressLint;
import android.app.Activity;
import android.os.Bundle;


@SuppressLint({ "SdCardPath", "HandlerLeak" })
public class HelpActivity extends Activity 
{
	@Override
	protected void onCreate(Bundle savedInstanceState) 
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_help);
	}
}