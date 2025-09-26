unit DrawPoint;

interface
uses Windows, Classes;
type
    TDrawPoint = class (TComponent)
    private
        m_X, m_Y: integer;
        function getX:integer;//virtual;
        function getEndX:integer;//virtual;
        function getY:integer;//virtual;
        function getPoint:TPoint;//virtual;

    protected
        procedure setPoint(Point:TPoint);virtual;
        procedure setX(x: integer);virtual;
        procedure setY(y: integer);virtual;
        procedure setPlace(x,y: integer);virtual;
        function getLength:integer;virtual;abstract;
    public
    //---------Propertyes
        procedure Move(dx,dy:integer);
        Property Point:TPoint Read getPoint Write setPoint;
        Property  EndX: integer Read getEndX;
        Property  EndY: integer Read getY;
    Published
        Property  x: integer Read getX Write SetX;
        Property  y: integer Read getY Write SetY;
    end;

implementation
procedure TDrawPoint.Move(dx,dy:integer);
begin
  m_x:=x+dx;
  m_y:=y+dy;
end;
function TDrawPoint.getX:integer;
begin
     result:=m_X;
end;
function TDrawPoint.getEndX:integer;
begin
     result:=m_X+getLength;
end;
function TDrawPoint.getY:integer;
begin
     result:=m_Y;
end;
function TDrawPoint.getPoint:TPoint;
Var
   Point:TPoint;
begin
     Point.x:=m_X;
     Point.y:=m_Y;
     result:=Point;
end;

procedure TDrawPoint.setX(x: integer);
begin
     m_X:=x;
end;
procedure TDrawPoint.setY(y: integer);
begin
     m_Y:=y;
end;
procedure TDrawPoint.setPlace(x,y: integer);
begin
     m_X:=x;
     m_Y:=y;
end;
procedure TDrawPoint.setPoint(Point: TPoint);
begin
     setX(Point.x);
     setY(Point.y);
end;
end.
