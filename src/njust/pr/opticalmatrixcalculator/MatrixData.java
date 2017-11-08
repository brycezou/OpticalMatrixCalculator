package njust.pr.opticalmatrixcalculator;

public class MatrixData 
{
	public int m_nRow;
	public int m_nCol;
	public double[][] m_matrix;
	
	public MatrixData(int w, int h)
	{
		m_nRow = w;
		m_nCol = h;
		m_matrix = new double[m_nRow][m_nCol];
	}
}
