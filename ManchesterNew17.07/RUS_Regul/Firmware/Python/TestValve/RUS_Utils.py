from math import pi, sin, cos, atan2, sqrt
from datetime import datetime, timedelta
import time
import lasio

class RVect:        # Плоский вектор в навигационных полярных координатах (длина R, поворот на угол Fi от направления "вверх/север", по часовой стрелке)
    """ Операции:
    - сложить с другим вектором
    - умножить длину на число
    - повернуть на угол
    - преобразовать в прямоугольные координаты XY (две оси под 90°, X направо, Y вверх)
    - преобразовать в математические полярные координаты RFi (длина R, поворот на угол Fi от направления "направо", против часовой стрелки)
    - преобразовать в навигационные координаты UVW (три оси под 120°, U вверх, V от U по часовой на 120°, W от U по часовой на 240°)
    """

    def __init__( self, R, Fi ):
        # Инициализация радиусом и углом
        self.R = R
        self.Fi = Fi % ( 2 * pi )
        
    def FromPol( R, Fi ):
        # Преобразование из математических полярных координат
        return RVect( R, pi/2 - Fi )
        
    def FromXY( x, y ):
        # Создание вектора через координаты X, Y (в прямоугольных координатах)
        return RVect.FromPol( sqrt( x*x + y*y ), atan2( y, x ) )
        
    def FromUVW( u, v, w ):
        # Создание вектора через координаты U, V, W
        ru = RVect( u, 0 )
        rv = RVect( v, +pi*2/3 )
        rw = RVect( w, -pi*2/3 )
        return ru + rv + rw

    def ToPol( self ):
        # Преобразовать в полярные координаты R, Fi
        return ( self.R, pi/2 - self.Fi )
        
    def ToXY( self ):
        # Преобразовать в прямоугольные координаты XY (две оси под 90°)
        x = self.R * sin( self.Fi )
        y = self.R * cos( self.Fi )
        return ( x, y )
        
    def ToUVW3( self, RAvg = None ):
        # Преобразовать в координаты UVW (три оси под 120°), обеспечивая среднюю величину трех векторов
        if RAvg == None:
            RAvg = self.R
        v = self.R * sin( self.Fi ) / sqrt(3) + RAvg - self.R * cos( self.Fi ) / 3
        w = 2 * RAvg - v - self.R * cos( self.Fi ) * 2/3
        u = 3 * RAvg - v - w
        if ( u < 0 ) or ( v < 0 ) or ( w < 0 ):
            return self.ToUVW2( )
        else:
            return ( u, v, w )
        
    def ToUVW2( self ):
        # Преобразовать в координаты UVW (три оси под 120°), разложить на два из трех векторов
        self.Fi %= 2*pi
        sin60 = sqrt( 3 ) / 2
        if self.Fi < pi*2/3 :
            u = self.R * sin( pi*2/3 - self.Fi ) / sin60
            v = self.R * sin( self.Fi ) / sin60
            w = 0
        elif self.Fi < pi*4/3 :
            u = 0
            v = self.R * sin( pi*4/3 - self.Fi ) / sin60
            w = self.R * sin( self.Fi - pi*2/3 ) / sin60
        else:
            u = self.R * sin( self.Fi - pi*4/3 ) / sin60
            v = 0
            w = self.R * sin( -self.Fi ) / sin60
        return ( u, v, w )

    def __mul__( self, rv ):
        # Перегрузка операции умножения на другой вектор: длины перемножить, углы сложить
        return RVect( self.R * rv.R, self.Fi + rv.Fi )
        
    def __add__( self, rv ):
        # Перегрузка операции сложения с другим вектором: обычное сложение векторов в прямоугольных координатах
        ( x1, y1 ) = self.ToXY()
        ( x2, y2 ) = rv.ToXY()
        return RVect.FromXY( x1 + x2, y1 + y2 )

    # Печать в строку в разных форматах
    def ToStrRFi( self ):
        return 'R = {:.2f}, Fi = {:.2f}°'.format( self.R, self.Fi * 180 / pi )
    def ToStrRFiPol( self ):
        return 'R = {:.2f}, Fi = {:.2f}°'.format( self.R, pi/2 - self.Fi * 180 / pi )
    def ToStrXY( self ):
        ( x, y ) = self.ToXY()
        return 'X = {:.2f}, Y = {:.2f}'.format( x, y )
    def ToStrUVW( self, RAvg = None ):
        ( u, v, w ) = self.ToUVW( RAvg )
        return 'U = {:.2f}, V = {:.2f}, W = {:.2f}'.format( u, v, w )
    def Print( self, RAvg = None ):
        print( 'RVect: {};\t{};\t{}'.format( self.ToStrRFi(), self.ToStrXY(), self.ToStrUVW( RAvg ) ) )

    # Проверка работы математики
    def Test():
        rv = RVect( 2, pi/4 )
        rv.Print( )
        rv2 = rv * RVect( 1.5, pi/2 )
        rv2.Print( )
        rv3 = rv+RVect( 2, -pi/4 )
        rv3.Print( )
        RVect.FromUVW( 1, 0, 0 ).Print( )
        RVect.FromUVW( 0, 1, 0 ).Print( )
        RVect.FromUVW( 0, 0, 1 ).Print( )
        RVect.FromUVW( 1, 1, 1 ).Print( )
        RVect( 1, pi/2 - pi * 2/3 ).Print( )
        RVect( 1, pi/2 + pi * 2/3 ).Print( )
        print( '' )
        rv1 = RVect.FromXY( 1, +0.577 )
        rv2 = RVect.FromXY( -1, +0.577 )
        rv1.Print()
        rv2.Print()
        ( rv1 + rv2 ).Print()
        print( '' )

    def Test2():
        RVect.FromUVW( 1, 0, 0 ).Print( )
        RVect.FromUVW( 0, 1, 0 ).Print( )
        RVect.FromUVW( 0, 0, 1 ).Print( )
        RVect.FromUVW( 1, 1, 1 ).Print( )
        RVect.FromUVW( 0, 1, 1 ).Print( 5 )

class RVectPlot:    # Пространство для отрисовки векторов
    def __init__( self, ax, rvOrigin = RVect( 0, 0 ), rvScale = RVect( 1, 0 ) ):
        self.ax = ax
        ( self.x0, self.y0 ) = rvOrigin.ToXY()  # начало координат
        self.rvScale = rvScale                  # поворот и масштабирование
        self.RMax = 0
    
    # Отрисовать вектор в память
    def Plot( self, rv, Color, Width = 0.1 ):
        ( x, y ) = ( rv * self.rvScale ).ToXY()
        self.RMax = max( self.RMax, abs( rv.R ) )
        return self.ax.quiver( self.x0, self.y0, x, y, color = Color, units = 'xy', scale = 1, scale_units = 'x', width = Width )
        # return self.ax.quiver( self.x0, self.y0, x, y, color = Color, units = 'xy', scale = 1, scale_units = 'x' )
        # self.ax.quiverkey( q, 0.5, 0.75, 1, '123456', labelpos='E' )
        
    # Подготовить диаграмму к выводу на экран
    def DrawPrepareFix( self ):
        self.ax.set_xlim( -3, 3 )
        self.ax.set_ylim( -3, 3 )
        self.ax.get_xaxis().set_visible( False )
        self.ax.get_yaxis().set_visible( False )

    # Подготовить диаграмму к выводу на экран
    def DrawPrepareAuto( self ):
        if self.RMax > 0:
            Lim = 1.1 * self.RMax
            self.ax.set_xlim( -Lim, +Lim )
            self.ax.set_ylim( -Lim, +Lim )
        self.ax.get_xaxis().set_visible( False )
        self.ax.get_yaxis().set_visible( False )
        self.RMax = 0

class SaverLAS:     # Сохранение накопленных данных в файл LAS
    # Класс реализует абстракцию сохранения в LAS, из обязательных полей сохраняется только Datetime в столбец 'TIME'.
    # Сохранение произвольного набора полей производится через функцию SaveCB(), которую необходимо определить в наследуемом классе.

    FileNamePostfix = datetime.now().strftime( '%Y%m%d %H%M%S' )
    FileNameTemplate= 'template.las'
    SavingInterval  = 5.0           # интервал сохранения в LAS

    def __init__( self, FileNameBase, atData, WorkDir = '.' ):
        self.WorkDir    = WorkDir   # папка с файлом 'template.las' и куда складывать файлы
        # self.FileName   = f'{SaverLAS.FileDir}\\{FileNameBase} {SaverLAS.FileNamePostfix}.las'    # 
        self.FileName   = f'{self.WorkDir}\\{FileNameBase} {SaverLAS.FileNamePostfix}.las'    # 
        self.FileLAS    = None      # файл, открытый через lasio.read( 'Template.las' ) - используется при первом сохранении для правильного заполнения шапки
        self.FileTxt    = None      # файл, открытый через open( 'a' ) - используется для дописывания новых данных в конец файла
        self.atData     = atData    # ссылка на массив atData[Time][Fields]- источник данных. Среди полей кортежа д.б. Datetime, остальные поля произвольные
        self.SavedIndex = -1        # индекс успешно сохраненных данных из массива
        self.SavingTimestamp = 0    # время последней записи, от которой отсчитывается интервал сохранения
        self.DateOrigin = 0         # дата первой записи в наборе данных (без времени)
        
    def GetTimeFromDatetime( self, Datetime ):       # вычислятор времени для столбца 'TIME'
        dDT = Datetime - self.DateOrigin
        return dDT.days*24*60*60*1000 + dDT.seconds*1000 + dDT.microseconds/1000

    def Save( self ):               # Выгрузить данные в LAS
        Timestamp = time.time()
        if ( Timestamp - self.SavingTimestamp ) < SaverLAS.SavingInterval:    # активировать запись только через определенные интервалы
            return
        self.SavingTimestamp = Timestamp
        atData = self.atData
        print( f'Saving data to {self.FileName}... ', end = '' )
        DataLenght = len( atData )
        if DataLenght <= 0:
            print( 'No data to save!' )
        else:
            if self.SavedIndex < 0:  # данные еще не выгружались - произвести первоначальное сохранение через библиотеку lasio
                DatetimeStart = atData[0].Datetime
                self.DateOrigin = datetime( DatetimeStart.year, DatetimeStart.month, DatetimeStart.day )
                # self.FileLAS = lasio.read( SaverLAS.FileNameTemplate, encoding='utf-8' )
                self.FileLAS = lasio.read( self.WorkDir + '\\' + SaverLAS.FileNameTemplate, encoding='utf-8' )
                self.FileLAS.well.DATE = '{:02d}/{:02d}/{:d}'.format( self.DateOrigin.day, self.DateOrigin.month, self.DateOrigin.year )
                self.FileLAS.append_curve( 'TIME', [ self.GetTimeFromDatetime( d.Datetime ) for d in atData ], descr = 'TIME, ms' )
                self.SaveCB( 0 )
                self.FileLAS.write( self.FileName, version = 2.0 )
                self.FileLAS = None
                self.FileTxt = open( self.FileName, 'a' )
            else:                   # данные уже выгружались - произвести добавление данных в конец открытого файла через обычный файловый вывод
                for i in range( self.SavedIndex+1, DataLenght ):   # выгрузить несохраненные данные построчно в конец файла
                    d = atData[i]
                    self.FileTxt.write( f'{self.GetTimeFromDatetime( d.Datetime ):.5f}' )
                    self.SaveCB( i )
                    self.FileTxt.write( '\n' )
                self.FileTxt.flush()
            self.SavedIndex = DataLenght - 1
            print( f'Ok, { ( time.time() - Timestamp ):0.3f} s passed.' )

    def SaveCB( self, FromIndex ):  # Прототип функции, вызываемой из SaverLAS.Save() для сохранения требуемого набора данных; реализуется в наследуемом классе
        if FromIndex == 0:  # данные еще не выгружались - произвести первоначальное сохранение через библиотеку lasio
            # Файл LAS уже открыт через lasio.read() и проинициализирован, столбец времени добавлен.
            # Требуется добавить остальные столбцы. Сохранение будет завершено после завершения.
            pass
            # Например:
            self.FileLAS.append_curve( 'TFG_Incl',       [ d.InclTF          for d in self.atData ],       descr = 'Inclin Toolface, °' )
        else:               # данные уже выгружались - произвести добавление данных в конец открытого файла через обычный файловый вывод
            # Выгрузить одну строку несохраненных данных.
            # Файл LAS уже открыт как текст через open( 'a' ), столбец времени заполнен.
            # Требуется добавить остальные столбцы. Сохранение будет завершено после завершения через flush()
            pass
            # Например:
            # d = self.atData[ FromIndex ]
            for Tag in ( d.InclTF ):
                self.FileTxt.write( f'\t{Tag:.5f}' )
 