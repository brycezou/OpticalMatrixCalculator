package njust.pr.opticalmatrixcalculator;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;

import org.json.JSONException;
import org.json.JSONObject;
import org.json.JSONTokener;

import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.provider.MediaStore;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.util.DisplayMetrics;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup.LayoutParams;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.TextView;
import android.widget.Toast;

import Jama.Matrix;

@SuppressLint({ "SdCardPath", "HandlerLeak" })
public class MainActivity extends Activity 
{
	private TableLayout m_tableA = null;
	private TableLayout m_tableB = null;
	private TableLayout m_tableC = null;

	private Bitmap m_bmpOrigIn = null;
	private boolean m_bIsProcessingImg = false;
	private boolean m_bSDCardAvalible = false;
	
	private Spinner m_spinnerA = null; 
	private Spinner m_spinnerB = null; 
	private Spinner m_spinnerC = null; 
	private ArrayAdapter<String> m_adapterA = null;
	private ArrayAdapter<String> m_adapterB = null;
	private ArrayAdapter<String> m_adapterC = null;
	//choose,inverse,rank,squre,cube,determinant,eigenvalues
	private static final String[] m_strArrayA = {"请选择对矩阵A的操作","A的逆","A的秩","A的平方","A的立方","A的行列式","A的特征值和特征向量"}; 
	private static final String[] m_strArrayB = {"请选择对矩阵B的操作","B的逆","B的秩","B的平方","B的立方","B的行列式","B的特征值和特征向量"}; 
	private static final String[] m_strArrayC = {"请选择对两个矩阵的操作","A + B","A - B","A x B","A-1 x B", "B x A-1"}; 
	
	private ArrayList<EditText> m_listEditTextA = null;
	private ArrayList<EditText> m_listEditTextB = null;
	private ArrayList<EditText> m_listEditTextC = null;
	
	public MatrixData m_MatrixA = null;
	public MatrixData m_MatrixB = null;
	public MatrixData m_MatrixC = null;
	public TextView m_tvToShow = null;
	public int m_ScreenWidth = 0;
	public int m_firstTimeCount = 0;
	
	private final int TAKE_PHOTO_A = 11;
	private final int TAKE_PHOTO_B = 22;
	private final int IMAGE_A = 33;
	private final int IMAGE_B = 44;

	
	static 
	{
		System.loadLibrary("JniMatrixRecognizer");
	}
	public native MatrixInfo RecognizeMatrix(Bitmap bmpIn);
	
	
	@Override
	protected void onCreate(Bundle savedInstanceState) 
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);  
		
		if(Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) 
		{
			m_bSDCardAvalible = true;	
			if(isFolderExists("/sdcard/MatrixCalculator/"))
		    {
				if(!copyFile("fannModel", "/sdcard/MatrixCalculator/fannModel"))
					Toast.makeText(this, "创建fannModel失败!", Toast.LENGTH_SHORT).show();  
		    }       
		}
		else
		{
			m_bSDCardAvalible = false;
			OnShowDlg("SD卡异常, 无法使用拍照计算功能!");
		}
		
		DisplayMetrics dm = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(dm);
		m_ScreenWidth = dm.widthPixels;
		onInitialViews();
	}
	private void onInitialViews()
	{	
		m_listEditTextA = new ArrayList<EditText>();
		m_listEditTextB = new ArrayList<EditText>();
		//m_listEditTextC = new ArrayList<EditText>();
		
		m_tvToShow = (TextView) findViewById(R.id.tvLabel);
		
		//TableLayout
		m_tableA = (TableLayout) findViewById(R.id.tableA);  
		m_tableA.setStretchAllColumns(true); 
		m_tableB = (TableLayout) findViewById(R.id.tableB);  
		m_tableB.setStretchAllColumns(true); 
		m_tableC = (TableLayout) findViewById(R.id.tableC);  
		m_tableC.setStretchAllColumns(true); 
		onCreateMatrixTable(m_tableA, 5, 5);
		onCreateMatrixTable(m_tableB, 5, 5);
		
		//SpinnerA
		m_spinnerA = (Spinner) findViewById(R.id.SpinnerA); 
		m_adapterA = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, m_strArrayA);
		m_adapterA.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);  
		m_spinnerA.setAdapter(m_adapterA);
		m_spinnerA.setOnItemSelectedListener(new SpinnerASelectedListener());
		m_spinnerA.setVisibility(View.VISIBLE);
		
		//SpinnerB
		m_spinnerB = (Spinner) findViewById(R.id.SpinnerB); 
		m_adapterB = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, m_strArrayB);
		m_adapterB.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);  
		m_spinnerB.setAdapter(m_adapterB);
		m_spinnerB.setOnItemSelectedListener(new SpinnerBSelectedListener());
		m_spinnerB.setVisibility(View.VISIBLE);
		
		//SpinnerC
		m_spinnerC = (Spinner) findViewById(R.id.SpinnerC); 
		m_adapterC = new ArrayAdapter<String>(this, android.R.layout.simple_spinner_item, m_strArrayC);
		m_adapterC.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);  
		m_spinnerC.setAdapter(m_adapterC);
		m_spinnerC.setOnItemSelectedListener(new SpinnerCSelectedListener());
		m_spinnerC.setVisibility(View.VISIBLE);
		
		View viewTemp = (View) findViewById(R.id.vFocus);
		viewTemp.setFocusable(true);
		viewTemp.requestFocus();
		viewTemp.setFocusableInTouchMode(true);
	}

	
	//create a nRow x nCol dynamic table in TableLayout tl
	private void onCreateMatrixTable(TableLayout tl, int nRow, int nCol)
	{
		int maxWidth = m_ScreenWidth/(10*nCol);
		switch(tl.getId())
		{
		case R.id.tableA:
			m_listEditTextA.clear();
			for(int y = 0; y < nRow; y++) 
			{  
	        	TableRow tablerow = new TableRow(MainActivity.this);  
	            //tablerow.setBackgroundColor(Color.rgb(222, 220, 210));  
	            for(int x = 0; x < nCol; x++) 
	            {  
	            	EditText tv = new EditText(MainActivity.this);
	                tv.setMaxWidth(maxWidth);
	                tv.setSingleLine();
	                tv.setText("0");  
	                tv.setSelectAllOnFocus(true);
	                tablerow.addView(tv);  
	                m_listEditTextA.add(tv);
	            }  
	            
	            tl.addView(tablerow, 
	            	new TableLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));  
			}
			m_MatrixA = new MatrixData(nRow, nCol);
			break;
		case R.id.tableB:
			m_listEditTextB.clear();
			for(int y = 0; y < nRow; y++) 
			{  
	        	TableRow tablerow = new TableRow(MainActivity.this);  
	            //tablerow.setBackgroundColor(Color.rgb(222, 220, 210));  
	            for(int x = 0; x < nCol; x++) 
	            {  
	            	EditText tv = new EditText(MainActivity.this);
	                tv.setMaxWidth(maxWidth);
	                tv.setSingleLine();
	                tv.setText("0");  
	                tv.setSelectAllOnFocus(true);
	                tablerow.addView(tv);  
	                m_listEditTextB.add(tv);
	            }  
	            
	            tl.addView(tablerow, 
	            	new TableLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));  
			}
			m_MatrixB = new MatrixData(nRow, nCol);
			break;
		}
	}
	//create a nRow x nCol dynamic table in TableLayout tl according to jsObject
	private void onCreateMatrixTable(TableLayout tl, int nRow, int nCol, JSONObject jsObject) throws JSONException
	{
		int maxWidth = m_ScreenWidth/(10*nCol);
		tl.removeAllViews();
		switch(tl.getId())
		{
		case R.id.tableA:
			m_listEditTextA.clear();
			for(int y = 0; y < nRow; y++) 
			{  
	        	TableRow tablerow = new TableRow(MainActivity.this);  
	            //tablerow.setBackgroundColor(Color.rgb(222, 220, 210));  
	            for(int x = 0; x < nCol; x++) 
	            {  
	            	EditText tv = new EditText(MainActivity.this);
	                tv.setMaxWidth(maxWidth);
	                tv.setSingleLine();
	            	tv.setText(jsObject.getDouble(y*nCol+x+"")+"");
	            	tv.setSelectAllOnFocus(true);
	                tablerow.addView(tv);  
	                m_listEditTextA.add(tv);
	            }  
	            tl.addView(tablerow, 
	            	new TableLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));  
			}
			m_MatrixA = null;
			m_MatrixA = new MatrixData(nRow, nCol);
			break;
		case R.id.tableB:
			m_listEditTextB.clear();
			for(int y = 0; y < nRow; y++) 
			{  
	        	TableRow tablerow = new TableRow(MainActivity.this);  
	            //tablerow.setBackgroundColor(Color.rgb(222, 220, 210));  
	            for(int x = 0; x < nCol; x++) 
	            {  
	            	EditText tv = new EditText(MainActivity.this);
	                tv.setMaxWidth(maxWidth);
	                tv.setSingleLine();
	            	tv.setText(jsObject.getDouble(y*nCol+x+"")+"");
	            	tv.setSelectAllOnFocus(true);
	                tablerow.addView(tv);  
	                m_listEditTextB.add(tv);
	            }  
	            tl.addView(tablerow, 
	            	new TableLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));  
			}
			m_MatrixB = null;
			m_MatrixB = new MatrixData(nRow, nCol);
			break;
		}
	}
	
	
	//load matrix image button clicked
	public void onLoadBtnClicked(View view)
	{
		if(!m_bSDCardAvalible)
		{
			OnShowDlg("SD卡异常, 无法使用拍照计算功能!");
			return;
		}
		if(m_bIsProcessingImg)
		{
			OnShowDlg("请等待当前任务完成!");
			return;
		}
		Intent intent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
		Uri imageUri = Uri.fromFile(new File(Environment.getExternalStorageDirectory(),"/MatrixCalculator/matrixImage.jpg"));
        intent.putExtra(MediaStore.EXTRA_OUTPUT, imageUri);
        
		switch(view.getId())
		{
		case R.id.btnA:	
			startActivityForResult(intent, TAKE_PHOTO_A);
			break;
		case R.id.btnB:
			startActivityForResult(intent, TAKE_PHOTO_B);
			break;
		}
	}
	private void startCutImage(Uri uri, int which) 
	{
        Intent intent = new Intent("com.android.camera.action.CROP");
        intent.setDataAndType(uri, "image/*");
        //crop为true, 设置intent中的view可以剪裁
        intent.putExtra("crop", "true");
        //aspectX, aspectY, 是宽高的比例
        intent.putExtra("aspectX", 1);
        intent.putExtra("aspectY", 1);

        //outputX,outputY 是剪裁图片的宽高
        intent.putExtra("outputX", 222);
        intent.putExtra("outputY", 222);
        intent.putExtra("return-data", true);
        intent.putExtra("noFaceDetection", true);
        startActivityForResult(intent, which);
    }
	@Override
	protected void onActivityResult(int requestCode, int resultCode, Intent data)
	{
		switch(requestCode)
		{
		case TAKE_PHOTO_A:
			if(resultCode == Activity.RESULT_OK)
			{
				Uri imageUri = Uri.fromFile(new File(Environment.getExternalStorageDirectory(),"/MatrixCalculator/matrixImage.jpg"));
				startCutImage(imageUri, IMAGE_A);
			}
			break;
		case TAKE_PHOTO_B:
			if(resultCode == Activity.RESULT_OK)
			{
				Uri imageUri = Uri.fromFile(new File(Environment.getExternalStorageDirectory(),"/MatrixCalculator/matrixImage.jpg"));
				startCutImage(imageUri, IMAGE_B);
			}
			break;
		case IMAGE_A:
			if(data != null)
			{                  
				processImage(data, IMAGE_A);              
			}
			break;
		case IMAGE_B:
			if(data != null)
			{                  
				processImage(data, IMAGE_B);              
			}
			break;
		default:
			break;
		}
		
		super.onActivityResult(requestCode, resultCode, data);
	}
	private void processImage(Intent picdata, final int which)
	{      
		if(m_bmpOrigIn != null)
			m_bmpOrigIn.recycle();
		
		Bundle extras = picdata.getExtras();          
		if (extras != null) 
		{       
			m_bmpOrigIn = extras.getParcelable("data");   
			saveBitmap(m_bmpOrigIn);
			new Thread( new Runnable() { public void run()
			{
				m_bIsProcessingImg = true;
				MatrixInfo mi;
				Bundle data = new Bundle();
				try
				{
					mi = RecognizeMatrix(m_bmpOrigIn);
					data.putString("result", mi.strMatrix);
				}
				catch(Exception e)
				{
					data.putString("result", "{\"end\":\"error\"}");
				}
				Message message = handler.obtainMessage();
				switch(which)
				{
				case IMAGE_A:			
					message.what = IMAGE_A;
					break;
				case IMAGE_B:			
					message.what = IMAGE_B;
					break;
				}
				message.setData(data);
				handler.sendMessage(message);
			}}).start();
		}
	}
	public void saveBitmap(Bitmap bmp) 
	{ 
		File f = new File("/sdcard/MatrixCalculator/ToProcess.jpg"); 
		try 
		{ 
			FileOutputStream out = new FileOutputStream(f); 
			bmp.compress(Bitmap.CompressFormat.JPEG, 90, out); 
			out.flush(); 
			out.close(); 
		} 
		catch (IOException e) 
		{ 
			OnShowDlg("图片222x222保存失败!"); 
		} 
	} 
	public Handler handler = new Handler()
	{
		@Override
		public void handleMessage(Message msg)
		{
			String strResult = msg.getData().getString("result");
			JSONObject jsObject;
			try 
			{
				jsObject = (JSONObject) new JSONTokener(strResult).nextValue();
				if(jsObject.getString("end").equals("ok"))
				{
					int nRow = jsObject.getInt("row");
					int nCol = jsObject.getInt("col");
					switch(msg.what)
					{
					case IMAGE_A:
						onCreateMatrixTable(m_tableA, nRow, nCol, jsObject);
						break;
					case IMAGE_B:
						onCreateMatrixTable(m_tableB, nRow, nCol, jsObject);
						break;
					}
				}
				else
				{
					OnShowDlg("矩阵识别错误, 请重新拍摄!");
				}
			}
			catch (JSONException e) 
			{
				OnShowDlg("Json解析失败!");
			}    	
			m_bIsProcessingImg = false;
		}
	};
	
	
	boolean isFolderExists(String strFolder)
	 {
		 File file = new File(strFolder);     
		 if (!file.exists())
		 {
			 if (file.mkdir())
			 {
				 return true;
			 }
			 else
				 return false;
		 }
		 return true;
	}
    private boolean copyFile(String fileFromName, String toDir) 
    {  
        try 
        {  
            InputStream its = getAssets().open(fileFromName);  
            int fileLength = its.available();  
            File book_file = new File(toDir);  
            if(book_file.exists())
            {
            	its.close(); 
            	return true;
            }
            book_file.createNewFile();  

            FileOutputStream fots = new FileOutputStream(book_file, true);   
            byte[] buffer = new byte[fileLength];  
            int readCount = 0;
            while (readCount < fileLength) 
            {  
                readCount += its.read(buffer, readCount, fileLength - readCount);  
            }  
            fots.write(buffer, 0, fileLength);  
      
            its.close();  
            fots.close();  
            
            return true;  
        } 
        catch (IOException e)
        {  
            return false;  
        }  
    }  
	public void OnShowDlg(String msg)
	{         
    	Toast.makeText(getApplicationContext(), msg, Toast.LENGTH_SHORT).show();
    }
	
	
	//get matrix elements from table
	public boolean getMatrixA()
	{
		if(m_listEditTextA.size() == 25)
		{
			m_MatrixA.m_matrix = null;
			m_MatrixA.m_matrix = new double[5][5];
			int imax = -1;
			int jmax = -1;
			for(int i = 0; i < 5; i++)
			{
				for(int j = 0; j < 5; j++)
				{
					try
					{
						String strNumber = ((EditText)m_listEditTextA.get(i*5+j)).getText().toString().trim();
						m_MatrixA.m_matrix[i][j] = Double.parseDouble(strNumber);
						if(m_MatrixA.m_matrix[i][j] != 0.0)
						{
							imax = imax > i ? imax : i;
							jmax = jmax > j ? jmax : j;
						}
					}
					catch(Exception e)
					{
						return false;
					}
				}
			}
			
			if(imax == -1 && jmax == -1)
			{
				///////////////////////////////
			}
			else
			{
				m_MatrixA.m_matrix = null;
				m_MatrixA.m_nRow = imax+1;
				m_MatrixA.m_nCol = jmax+1;
				m_MatrixA.m_matrix = new double[m_MatrixA.m_nRow][m_MatrixA.m_nCol];
				for(int i = 0; i < m_MatrixA.m_nRow; i++)
				{
					for(int j = 0; j < m_MatrixA.m_nCol; j++)
					{
						String strNumber = ((EditText)m_listEditTextA.get(i*5+j)).getText().toString().trim();
						m_MatrixA.m_matrix[i][j] = Double.parseDouble(strNumber);
					}
				}
			}				
		}
		else
		{
			m_MatrixA.m_matrix = null;
			m_MatrixA.m_matrix = new double[m_MatrixA.m_nRow][m_MatrixA.m_nCol];
			//send number in tables into double array
			for(int i = 0; i < m_MatrixA.m_nRow; i++)
			{
				for(int j = 0; j < m_MatrixA.m_nCol; j++)
				{
					try
					{
						String strNumber = ((EditText)m_listEditTextA.get(i*m_MatrixA.m_nCol+j)).getText().toString().trim();
						m_MatrixA.m_matrix[i][j] = Double.parseDouble(strNumber);
					}
					catch(Exception e)
					{
						return false;
					}
				}
			}
		}
		return true;
	}
	public boolean getMatrixB()
	{
		if(m_listEditTextB.size() == 25)
		{
			m_MatrixB.m_matrix = null;
			m_MatrixB.m_matrix = new double[5][5];
			int imax = -1;
			int jmax = -1;
			for(int i = 0; i < 5; i++)
			{
				for(int j = 0; j < 5; j++)
				{
					try
					{
						String strNumber = ((EditText)m_listEditTextB.get(i*5+j)).getText().toString().trim();
						m_MatrixB.m_matrix[i][j] = Double.parseDouble(strNumber);
						if(m_MatrixB.m_matrix[i][j] != 0.0)
						{
							imax = imax > i ? imax : i;
							jmax = jmax > j ? jmax : j;
						}
					}
					catch(Exception e)
					{
						return false;
					}
				}
			}
			
			if(imax == -1 && jmax == -1)
			{
				///////////////////////////////
			}
			else
			{
				m_MatrixB.m_matrix = null;
				m_MatrixB.m_nRow = imax+1;
				m_MatrixB.m_nCol = jmax+1;
				m_MatrixB.m_matrix = new double[m_MatrixB.m_nRow][m_MatrixB.m_nCol];
				for(int i = 0; i < m_MatrixB.m_nRow; i++)
				{
					for(int j = 0; j < m_MatrixB.m_nCol; j++)
					{
						String strNumber = ((EditText)m_listEditTextB.get(i*5+j)).getText().toString().trim();
						m_MatrixB.m_matrix[i][j] = Double.parseDouble(strNumber);
					}
				}
			}				
		}
		else
		{
			m_MatrixB.m_matrix = null;
			m_MatrixB.m_matrix = new double[m_MatrixB.m_nRow][m_MatrixB.m_nCol];
			//send number in tables into double array
			for(int i = 0; i < m_MatrixB.m_nRow; i++)
			{
				for(int j = 0; j < m_MatrixB.m_nCol; j++)
				{
					try
					{
						String strNumber = ((EditText)m_listEditTextB.get(i*m_MatrixB.m_nCol+j)).getText().toString().trim();
						m_MatrixB.m_matrix[i][j] = Double.parseDouble(strNumber);
					}
					catch(Exception e)
					{
						return false;
					}
				}
			}
		}
		return true;
	}
		
	
	////////////////////////////////////////////////////////////////////////////////////列表框A的点击事件处理类
	class SpinnerASelectedListener implements OnItemSelectedListener
	{          
		public void onItemSelected(AdapterView<?> arg0, View arg1, int arg2, long arg3) 
		{   
			if(m_firstTimeCount < 3)
			{
				m_firstTimeCount++;
				return;
			}
			
			if(!getMatrixA())
			{
				OnShowDlg("矩阵A元素不合法!");
				m_spinnerA.setSelection(0);
				return;
			}

			
			//{"A的逆","A的秩","A的平方","A的立方","A的行列式","A的特征值和特征向量"}; 
			switch(arg2)
			{
			case 1:
				OnShowDlg("A的逆");
				if(SingleMatrixInverse(m_MatrixA.m_matrix))
					m_tvToShow.setText("矩阵A的逆: ");
				else
				{
					m_tableC.removeAllViews();
					m_tvToShow.setText("矩阵A的逆计算失败!");
				}
				m_spinnerA.setSelection(0);
				break;
			case 2:
				OnShowDlg("A的秩");
				m_tableC.removeAllViews();
				int rank = SingleMatrixRank(m_MatrixA.m_matrix);
				m_tvToShow.setText("矩阵A的秩: "+rank);
				m_spinnerA.setSelection(0);
				break;
			case 3:
				OnShowDlg("A的平方");
				if(SingleMatrixSquare(m_MatrixA.m_matrix))
					m_tvToShow.setText("矩阵A的平方: ");
				else
				{
					m_tableC.removeAllViews();
					m_tvToShow.setText("不能计算矩阵A的平方!");
				}
				m_spinnerA.setSelection(0);
				break;
			case 4:
				OnShowDlg("A的立方");
				if(SingleMatrixCube(m_MatrixA.m_matrix))
					m_tvToShow.setText("矩阵A的立方: ");
				else
				{
					m_tableC.removeAllViews();
					m_tvToShow.setText("不能计算矩阵A的立方!");
				}
				m_spinnerA.setSelection(0);
				break;
			case 5:
				OnShowDlg("A的行列式");
				m_tableC.removeAllViews();
				double mdet = SingleMatrixDeterminant(m_MatrixA.m_matrix);
				if(Math.abs(-111111.111111-mdet) < 1e-6)
					m_tvToShow.setText("不能计算矩阵A的行列式!");
				else
					m_tvToShow.setText("矩阵A的行列式: "+String.format("%.4f", mdet));
				m_spinnerA.setSelection(0);
				break;
			case 6:
				OnShowDlg("A的特征值和特征向量");
				if(SingleMatrixEigenvalueVector(m_MatrixA.m_matrix))
					m_tvToShow.setText("矩阵A的特征值和特征向量: ");
				else
				{
					m_tableC.removeAllViews();
					m_tvToShow.setText("不能计算矩阵A的特征值和特征向量!");
				}
				m_spinnerA.setSelection(0);
				break;
			}
		}
		
		@Override
		public void onNothingSelected(AdapterView<?> arg0) 
		{
			// TODO Auto-generated method stub
		} 
	}
	
	
	////////////////////////////////////////////////////////////////////////////////////列表框B的点击事件处理类
	class SpinnerBSelectedListener implements OnItemSelectedListener
	{           
		public void onItemSelected(AdapterView<?> arg0, View arg1, int arg2, long arg3) 
		{     
			if(m_firstTimeCount < 3)
			{
				m_firstTimeCount++;
				return;
			}
			
			if(!getMatrixB())
			{
				OnShowDlg("矩阵B元素不合法!");
				m_spinnerB.setSelection(0);
				return;
			}
			
			switch(arg2)
			{
			case 1:
				OnShowDlg("B的逆");
				if(SingleMatrixInverse(m_MatrixB.m_matrix))
					m_tvToShow.setText("矩阵B的逆: ");
				else
				{
					m_tableC.removeAllViews();
					m_tvToShow.setText("矩阵B的逆计算失败!");
				}
				m_spinnerB.setSelection(0);
				break;
			case 2:
				OnShowDlg("B的秩");
				m_tableC.removeAllViews();
				int rank = SingleMatrixRank(m_MatrixB.m_matrix);
				m_tvToShow.setText("矩阵B的秩: "+rank);
				m_spinnerB.setSelection(0);
				break;
			case 3:
				OnShowDlg("B的平方");
				if(SingleMatrixSquare(m_MatrixB.m_matrix))
					m_tvToShow.setText("矩阵B的平方: ");
				else
				{
					m_tableC.removeAllViews();
					m_tvToShow.setText("不能计算矩阵B的平方!");
				}
				m_spinnerB.setSelection(0);
				break;
			case 4:
				OnShowDlg("B的立方");
				if(SingleMatrixCube(m_MatrixB.m_matrix))
					m_tvToShow.setText("矩阵B的立方: ");
				else
				{
					m_tableC.removeAllViews();
					m_tvToShow.setText("不能计算矩阵B的立方!");
				}
				m_spinnerB.setSelection(0);
				break;
			case 5:
				OnShowDlg("B的行列式");
				m_tableC.removeAllViews();
				double mdet = SingleMatrixDeterminant(m_MatrixB.m_matrix);
				if(Math.abs(-111111.111111-mdet) < 1e-6)
					m_tvToShow.setText("不能计算矩阵B的行列式!");
				else
					m_tvToShow.setText("矩阵B的行列式: "+String.format("%.4f", mdet));
				m_spinnerB.setSelection(0);
				break;
			case 6:
				OnShowDlg("B的特征值和特征向量");
				if(SingleMatrixEigenvalueVector(m_MatrixB.m_matrix))
					m_tvToShow.setText("矩阵B的特征值和特征向量: ");
				else
				{
					m_tableC.removeAllViews();
					m_tvToShow.setText("不能计算矩阵B的特征值和特征向量!");
				}
				m_spinnerB.setSelection(0);
				break;
			}
		}

		@Override
		public void onNothingSelected(AdapterView<?> arg0) 
		{
			// TODO Auto-generated method stub
		} 
	}
	
	
	boolean SingleMatrixInverse(double mtx[][])
	{
		Matrix mm = new Matrix(mtx);
		if(mm.getColumnDimension()!=mm.getRowDimension())
			return false;
		Matrix inv;
		try
		{
			inv = mm.inverse();
		}
		catch(Exception e)
		{
			return false;
		}
		int row = inv.getRowDimension();
		int col = inv.getColumnDimension();
		double[][] data = inv.getArray();
		
		int maxWidth = m_ScreenWidth/(10*col);
		m_tableC.removeAllViews();
		//m_listEditTextC.clear();
		for(int y = 0; y < row; y++) 
		{  
	        TableRow tablerow = new TableRow(MainActivity.this);  
	        //tablerow.setBackgroundColor(Color.rgb(220, 255, 220));  
	        for(int x = 0; x < col; x++) 
	        {  
	        	EditText tv = new EditText(MainActivity.this);
                tv.setMaxWidth(maxWidth);
                tv.setSingleLine();
	        	tv.setText(String.format("%.4f", data[y][x]));
	        	
	        	tablerow.addView(tv);  
	        	//m_listEditTextC.add(tv);
	        }  
	        m_tableC.addView(tablerow, 
	        		new TableLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));  
		}
		//m_MatrixC = null;
		//m_MatrixC = new MatrixData(row, col);
		//m_MatrixC.m_matrix = data;
		return true;
	}
	int SingleMatrixRank(double mtx[][])
	{
		Matrix mm = new Matrix(mtx);
		int rank = mm.rank();
		return rank;
	}
	boolean SingleMatrixSquare(double mtx[][])
	{
		Matrix mm = new Matrix(mtx);
		Matrix msquare;
		try
		{
			msquare = mm.times(mm);
		}
		catch(Exception e)
		{
			return false;
		}
		int row = msquare.getRowDimension();
		int col = msquare.getColumnDimension();
		double[][] data = msquare.getArray();
		
		int maxWidth = m_ScreenWidth/(10*col);
		m_tableC.removeAllViews();
		for(int y = 0; y < row; y++) 
		{  
	        TableRow tablerow = new TableRow(MainActivity.this);  
	        //tablerow.setBackgroundColor(Color.rgb(220, 255, 220));  
	        for(int x = 0; x < col; x++) 
	        {  
	        	EditText tv = new EditText(MainActivity.this);
                tv.setMaxWidth(maxWidth);
                tv.setSingleLine();
	        	tv.setText(String.format("%.4f", data[y][x]));
	        	tablerow.addView(tv);  
	        }  
	        m_tableC.addView(tablerow, 
	        		new TableLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));  
		}
		return true;
	}
	boolean SingleMatrixCube(double mtx[][])
	{
		Matrix mm = new Matrix(mtx);
		Matrix mcube;
		try
		{
			mcube = mm.times(mm);
			mcube = mcube.times(mm);
		}
		catch(Exception e)
		{
			return false;
		}
		int row = mcube.getRowDimension();
		int col = mcube.getColumnDimension();
		double[][] data = mcube.getArray();
		
		int maxWidth = m_ScreenWidth/(10*col);
		m_tableC.removeAllViews();
		for(int y = 0; y < row; y++) 
		{  
	        TableRow tablerow = new TableRow(MainActivity.this);  
	        //tablerow.setBackgroundColor(Color.rgb(220, 255, 220));  
	        for(int x = 0; x < col; x++) 
	        {  
	        	EditText tv = new EditText(MainActivity.this);
                tv.setMaxWidth(maxWidth);
                tv.setSingleLine();
	        	tv.setText(String.format("%.4f", data[y][x]));
	        	tablerow.addView(tv);  
	        }  
	        m_tableC.addView(tablerow, 
	        		new TableLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));  
		}
		return true;
	}
	double SingleMatrixDeterminant(double mtx[][])
	{
		Matrix mm = new Matrix(mtx);
		double mdet;
		try
		{
			mdet = mm.det();
		}
		catch(Exception e)
		{
			return -111111.111111;
		}
		return mdet;
	}
	boolean SingleMatrixEigenvalueVector(double mtx[][])
	{
		Matrix mm = new Matrix(mtx);
		if(mm.getColumnDimension() != mm.getRowDimension())
			return false;
		
		Matrix mEigenvalue;
		try
		{
			mEigenvalue = mm.eig().getD();
			int rowValue = mEigenvalue.getRowDimension();
			int colValue = mEigenvalue.getColumnDimension();
			double[][] dataValue = mEigenvalue.getArray();	

			int maxWidth = m_ScreenWidth/(10*colValue);
			m_tableC.removeAllViews();
			for(int y = 0; y < rowValue; y++) 
			{  
		        TableRow tablerow = new TableRow(MainActivity.this);  
		        //tablerow.setBackgroundColor(Color.rgb(220, 255, 220));  
		        for(int x = 0; x < colValue; x++) 
		        {  
		        	EditText tv = new EditText(MainActivity.this);
	                tv.setMaxWidth(maxWidth);
	                tv.setSingleLine();
		        	tv.setText(String.format("%.4f", dataValue[y][x]));
		        	tablerow.addView(tv);  
		        }  
		        m_tableC.addView(tablerow, 
		        		new TableLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));  
			}
		}
		catch(Exception e)
		{
			return false;
		}

		
		Matrix mEigenvector;
		try
		{
			mEigenvector = mm.eig().getV();
			int rowVector = mEigenvector.getRowDimension();
			int colVector = mEigenvector.getColumnDimension();
			double[][] dataVector = mEigenvector.getArray();
			
			int maxWidth = m_ScreenWidth/(10*colVector);
			for(int y = 0; y < rowVector; y++) 
			{  
		        TableRow tablerow = new TableRow(MainActivity.this);   
		        tablerow.setBackgroundColor(Color.argb(100, 255, 200, 200));
		        for(int x = 0; x < colVector; x++) 
		        {  
		        	EditText tv = new EditText(MainActivity.this);
	                tv.setMaxWidth(maxWidth);
	                tv.setSingleLine();
		        	tv.setText(String.format("%.4f", dataVector[y][x]));
		        	tablerow.addView(tv);  
		        }  
		        m_tableC.addView(tablerow, 
		        		new TableLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));  
			}
		}
		catch(Exception e)
		{
			return false;
		}
		
		return true;
	}
	
	boolean TwoMatrixAdd(double mtxA[][], double mtxB[][])
	{
		Matrix mmA = new Matrix(mtxA);
		Matrix mmB = new Matrix(mtxB);
		Matrix mmSum;
		try
		{
			mmSum = mmA.plus(mmB);
		}
		catch(Exception e)
		{
			return false;
		}
		int row = mmSum.getRowDimension();
		int col = mmSum.getColumnDimension();
		double[][] data = mmSum.getArray();
		
		int maxWidth = m_ScreenWidth/(10*col);
		m_tableC.removeAllViews();
		for(int y = 0; y < row; y++) 
		{  
	        TableRow tablerow = new TableRow(MainActivity.this);  
	        //tablerow.setBackgroundColor(Color.rgb(220, 255, 220));  
	        for(int x = 0; x < col; x++) 
	        {  
	        	EditText tv = new EditText(MainActivity.this);
                tv.setMaxWidth(maxWidth);
                tv.setSingleLine();
	        	tv.setText(String.format("%.4f", data[y][x]));
	        	
	        	tablerow.addView(tv);  
	        }  
	        m_tableC.addView(tablerow, 
	        		new TableLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));  
		}
		return true;
	}
	boolean TwoMatrixSub(double mtxA[][], double mtxB[][])
	{
		Matrix mmA = new Matrix(mtxA);
		Matrix mmB = new Matrix(mtxB);
		Matrix mmSub;
		try
		{
			mmSub = mmA.minus(mmB);
		}
		catch(Exception e)
		{
			return false;
		}
		int row = mmSub.getRowDimension();
		int col = mmSub.getColumnDimension();
		double[][] data = mmSub.getArray();
		
		int maxWidth = m_ScreenWidth/(10*col);
		m_tableC.removeAllViews();
		for(int y = 0; y < row; y++) 
		{  
	        TableRow tablerow = new TableRow(MainActivity.this);  
	        //tablerow.setBackgroundColor(Color.rgb(220, 255, 220));  
	        for(int x = 0; x < col; x++) 
	        {  
	        	EditText tv = new EditText(MainActivity.this);
                tv.setMaxWidth(maxWidth);
                tv.setSingleLine();
	        	tv.setText(String.format("%.4f", data[y][x]));
	        	
	        	tablerow.addView(tv);  
	        }  
	        m_tableC.addView(tablerow, 
	        		new TableLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));  
		}
		return true;
	}
	boolean TwoMatrixTimes(double mtxA[][], double mtxB[][])
	{
		Matrix mmA = new Matrix(mtxA);
		Matrix mmB = new Matrix(mtxB);
		Matrix mmMul;
		try
		{
			mmMul = mmA.times(mmB);
		}
		catch(Exception e)
		{
			return false;
		}
		int row = mmMul.getRowDimension();
		int col = mmMul.getColumnDimension();
		double[][] data = mmMul.getArray();
		
		int maxWidth = m_ScreenWidth/(10*col);
		m_tableC.removeAllViews();
		for(int y = 0; y < row; y++) 
		{  
	        TableRow tablerow = new TableRow(MainActivity.this);  
	        //tablerow.setBackgroundColor(Color.rgb(220, 255, 220));  
	        for(int x = 0; x < col; x++) 
	        {  
	        	EditText tv = new EditText(MainActivity.this);
                tv.setMaxWidth(maxWidth);
                tv.setSingleLine();
	        	tv.setText(String.format("%.4f", data[y][x]));
	        	
	        	tablerow.addView(tv);  
	        }  
	        m_tableC.addView(tablerow, 
	        		new TableLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));  
		}
		return true;
	}
	boolean InverseATimesB(double mtxA[][], double mtxB[][])
	{
		Matrix mmA = new Matrix(mtxA);
		if(mmA.getColumnDimension()!=mmA.getRowDimension())
			return false;
		Matrix mmB = new Matrix(mtxB);
		Matrix mmX;
		try
		{
			mmX = mmA.solve(mmB);
		}
		catch(Exception e)
		{
			return false;
		}
		int row = mmX.getRowDimension();
		int col = mmX.getColumnDimension();
		double[][] data = mmX.getArray();
		
		int maxWidth = m_ScreenWidth/(10*col);
		m_tableC.removeAllViews();
		for(int y = 0; y < row; y++) 
		{  
	        TableRow tablerow = new TableRow(MainActivity.this);  
	        //tablerow.setBackgroundColor(Color.rgb(220, 255, 220));  
	        for(int x = 0; x < col; x++) 
	        {  
	        	EditText tv = new EditText(MainActivity.this);
                tv.setMaxWidth(maxWidth);
                tv.setSingleLine();
	        	tv.setText(String.format("%.4f", data[y][x]));
	        	
	        	tablerow.addView(tv);  
	        }  
	        m_tableC.addView(tablerow, 
	        		new TableLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));  
		}
		return true;
	}
	boolean BTimesInverseA(double mtxA[][], double mtxB[][])
	{
		Matrix mmA = new Matrix(mtxA);
		if(mmA.getColumnDimension()!=mmA.getRowDimension())
			return false;
		Matrix mmB = new Matrix(mtxB);
		Matrix mmX;
		try
		{
			mmX = mmA.inverse();
			mmX = mmB.times(mmX);
		}
		catch(Exception e)
		{
			return false;
		}
		int row = mmX.getRowDimension();
		int col = mmX.getColumnDimension();
		double[][] data = mmX.getArray();
		
		int maxWidth = m_ScreenWidth/(10*col);
		m_tableC.removeAllViews();
		for(int y = 0; y < row; y++) 
		{  
	        TableRow tablerow = new TableRow(MainActivity.this);  
	        //tablerow.setBackgroundColor(Color.rgb(220, 255, 220));  
	        for(int x = 0; x < col; x++) 
	        {  
	        	EditText tv = new EditText(MainActivity.this);
                tv.setMaxWidth(maxWidth);
                tv.setSingleLine();
	        	tv.setText(String.format("%.4f", data[y][x]));
	        	
	        	tablerow.addView(tv);  
	        }  
	        m_tableC.addView(tablerow, 
	        		new TableLayout.LayoutParams(LayoutParams.MATCH_PARENT, LayoutParams.WRAP_CONTENT));  
		}
		return true;
	}
	
	
	////////////////////////////////////////////////////////////////////////////////////列表框C的点击事件处理类
	class SpinnerCSelectedListener implements OnItemSelectedListener
	{           
		public void onItemSelected(AdapterView<?> arg0, View arg1, int arg2, long arg3) 
		{         
			if(m_firstTimeCount < 3)
			{
				m_firstTimeCount++;
				return;
			}
			
			if(!getMatrixA())
			{
				OnShowDlg("矩阵A元素不合法!");
				m_spinnerC.setSelection(0);
				return;
			}
			if(!getMatrixB())
			{
				OnShowDlg("矩阵B元素不合法!");
				m_spinnerC.setSelection(0);
				return;
			}
			
			switch(arg2)
			{
			case 1:
				OnShowDlg("A + B");
				if(TwoMatrixAdd(m_MatrixA.m_matrix, m_MatrixB.m_matrix))
					m_tvToShow.setText("矩阵 A + B : ");
				else
				{
					m_tableC.removeAllViews();
					m_tvToShow.setText("矩阵 A + B 计算失败!");
				}
				m_spinnerC.setSelection(0);
				break;
			case 2:
				OnShowDlg("A - B");
				if(TwoMatrixSub(m_MatrixA.m_matrix, m_MatrixB.m_matrix))
					m_tvToShow.setText("矩阵 A - B : ");
				else
				{
					m_tableC.removeAllViews();
					m_tvToShow.setText("矩阵 A - B 计算失败!");
				}
				m_spinnerC.setSelection(0);
				break;
			case 3:
				OnShowDlg("A x B");
				if(TwoMatrixTimes(m_MatrixA.m_matrix, m_MatrixB.m_matrix))
					m_tvToShow.setText("矩阵 A x B : ");
				else
				{
					m_tableC.removeAllViews();
					m_tvToShow.setText("矩阵 A x B 计算失败!");
				}
				m_spinnerC.setSelection(0);
				break;
			case 4:
				OnShowDlg("A-1 x B");
				if(InverseATimesB(m_MatrixA.m_matrix, m_MatrixB.m_matrix))
					m_tvToShow.setText("矩阵 A-1 x B : ");
				else
				{
					m_tableC.removeAllViews();
					m_tvToShow.setText("矩阵 A-1 x B 计算失败!");
				}
				m_spinnerC.setSelection(0);
				break;
			case 5:
				OnShowDlg("B x A-1");
				if(BTimesInverseA(m_MatrixA.m_matrix, m_MatrixB.m_matrix))
					m_tvToShow.setText("矩阵 B x A-1 : ");
				else
				{
					m_tableC.removeAllViews();
					m_tvToShow.setText("矩阵 B x A-1 计算失败!");
				}
				m_spinnerC.setSelection(0);
				break;
			}
		}

		@Override
		public void onNothingSelected(AdapterView<?> arg0) 
		{
			// TODO Auto-generated method stub
		} 
	}


	@Override
	public boolean onCreateOptionsMenu(Menu menu) 
	{
		// TODO Auto-generated method stub
		menu.add(0, Menu.FIRST, 1, "关于我们");
		menu.add(1, Menu.FIRST+1, 2, "使用手册");
		menu.add(2, Menu.FIRST+2, 3, "标准计算器");
		return super.onCreateOptionsMenu(menu);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) 
	{
		// TODO Auto-generated method stub
		switch(item.getItemId())
		{
		case 1:
			new AlertDialog.Builder(MainActivity.this).setTitle("关于").setMessage("作者: 无有" +
				"\n扣扣: 771966081\n邮箱: 771966081@qq.com\n\n欢迎您反馈使用意见, 我们会尽快完善产品, 谢谢 !").
				setNeutralButton("确 定", new DialogInterface.OnClickListener(){ public 
				void onClick(DialogInterface dialog, int whichButton){} }).show();
			break;
		case 2:
			Intent intt = new Intent();
			intt.setClass(this, HelpActivity.class);
			startActivity(intt);
			break;
		case 3:
			try
			{
				Intent intent = new Intent();
				intent.setClassName("com.android.calculator2","com.android.calculator2.Calculator");
				startActivity(intent);
			}
			catch(Exception e)
			{
				OnShowDlg("无法调用系统计算器");
			}
			break;
		default:
			break;
		}
		return super.onOptionsItemSelected(item);
	}
		
}
