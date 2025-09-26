unit Arrow;

interface
uses
    Semantic, DrawPoint, Ward, Graphics;
const
     spaceLength=5;
     minArrowLength=2*spaceLength;
//     nakLen=30;
//     nakLen2=nakLen*2;
     spikeLength=7;
     spikeWidth=3;
     textHeigth=15;
//     spike2=spike*2;

     ctArrow=0;
     ctSemanticArrow=1;
type
    TArrow = class
    Private
        m_Length: integer;
        m_FromDO: TDrawPoint;
        m_Ward: integer;
    public
        constructor Create;
        constructor make(Ward:integer; FromDO: TDrawPoint);

        function Save:TDrawPoint;virtual;

        function Copy:TArrow;virtual;
        Procedure Draw(Canvas: TCanvas; Owner: TDrawPoint);virtual;
        procedure SetFromDO(FromDO: TDrawPoint);
        Function getFromDO:TDrawPoint;
        Function getLength:integer;virtual;

        property Ward:integer read m_Ward write m_Ward;
    end;
    TSemanticArrow = class (TArrow)
    private
        m_Semantics: TSemanticIDList;
    public
        function Save:TDrawPoint;override;
        function Copy:TArrow;override;
        constructor Make(Ward:integer; FromDO: TDrawPoint;Semantics:TSemanticIDList);
        Procedure Draw(Canvas: TCanvas; Owner: TDrawPoint);override;
    end;
{    TSemanticNArrow = class (TSemanticArrow)
        m_Ward: integer;
        Function getLength:integer;override;
        Procedure Draw(Canvas: TCanvas; Owner: TDrawPoint);override;
    end;
    TNArrow = class (TArrow)
        m_Ward: integer;
        Function getLength:integer;override;
        Procedure Draw(Canvas: TCanvas; Owner: TDrawPoint);override;
    end;}
implementation
uses RegularExpression;
function TArrow.Save:TDrawPoint;
begin
  writeln(ctArrow);
  writeln(m_Ward);
  result:=m_FromDO;
end;
function TSemanticArrow.Save:TDrawPoint;
begin
  writeln(ctSemanticArrow);
  writeln(m_Ward);
  m_Semantics.Save;
  result:=m_FromDO;
end;

Procedure DrawSpike(Canvas:TCanvas;BeginX,BeginY,EndX,EndY:integer);
var
  dx,dy:integer;
begin
  dx:=(EndX-BeginX)div 2;
  dy:=(EndY-BeginY)div 2;
  Canvas.Pie(EndX-spikeLength,EndY-spikeLength,EndX+spikeLength,EndY+spikeLength,
             BeginX+dy,BeginY-dX,BeginX-dy,BeginY+dX);
end;
Function TArrow.getFromDO:TDrawPoint;
begin
  result:=m_FromDO;
end;
procedure TArrow.SetFromDO(FromDO: TDrawPoint);
begin
  m_FromDO:=FromDO;
end;
constructor TArrow.Create();
begin
  m_Ward:=cwNONE;
  m_Length:=minArrowLength;
end;
constructor TArrow.make(Ward:integer; FromDO: TDrawPoint);
begin
  m_Ward:=Ward;
  m_Length:=minArrowLength;
  m_FromDO:=FromDO;
  if Ward<>cwNONE then inc(m_Length,SpikeLength);
end;

Function TArrow.getLength:integer;
begin
  result:=m_Length;
end;

function TArrow.Copy:TArrow;
var
  res:TArrow;
begin
  res:=TArrow.Create();
  res.m_Length:=m_Length;
  res.m_FromDO:=m_FromDO;
  res.m_Ward:=m_Ward;
  result:=res;
end;

function TSemanticArrow.Copy:TArrow;
var
  res:TSemanticArrow;
begin
  res:=TSemanticArrow.Create;
  res.m_Length:=m_Length;
  res.m_FromDO:=m_FromDO;
  res.m_Semantics:=m_Semantics.copy;
  res.m_Ward:=m_Ward;
  result:=res;
end;

constructor TSemanticArrow.make(ward:integer; FromDO:TDrawPoint; Semantics:TSemanticIDList);
begin
  m_Ward:=ward;
  m_Length:=minArrowLength+Semantics.getLength;
  m_FromDO:=FromDO;
  if Ward<>cwNONE then inc(m_Length,SpikeLength);
  m_Semantics:=Semantics;
end;
Procedure TArrow.Draw(Canvas: TCanvas; Owner: TDrawPoint);
begin
     Canvas.MoveTo(m_FromDO.endX, m_FromDO.endY);
     Canvas.LineTo(Owner.x, Owner.y);
     if m_Ward <> cwNONE then begin
         Canvas.Brush.Color:=clBlack;
         if m_Ward = cwFORWARD then
             DrawSpike(Canvas,m_FromDO.endX,m_FromDO.endY,Owner.x, Owner.y)
         else
             DrawSpike(Canvas,Owner.x, Owner.y,m_FromDO.endX,m_FromDO.endY);
     end;
end;
Procedure TSemanticArrow.Draw(Canvas: TCanvas; Owner: TDrawPoint);
begin
     if m_Ward = cwNONE then begin
         m_Semantics.draw(Canvas, m_FromDO.endX+spaceLength, m_FromDO.endY-textHeigth);
     end else begin
         if m_Ward = cwFORWARD then begin
             m_Semantics.draw(Canvas, m_FromDO.endX+spaceLength, m_FromDO.endY-textHeigth);
             Canvas.Brush.Color:=clBlack;
             DrawSpike(Canvas,m_FromDO.endX,m_FromDO.endY,Owner.x, Owner.y);
         end else begin
             m_Semantics.draw(Canvas, m_FromDO.endX+spaceLength+spikeLength, m_FromDO.endY-textHeigth);
             Canvas.Brush.Color:=clBlack;
             DrawSpike(Canvas,Owner.x, Owner.y,m_FromDO.endX,m_FromDO.endY);
         end;
     end;
     Canvas.MoveTo(m_FromDO.endX, m_FromDO.endY);
     Canvas.LineTo(Owner.x, Owner.y);
end;
{
Function TNArrow.getLength:integer;
begin
     result:=minArrowLength+spike;
end;
Procedure TNArrow.Draw(Canvas: TCanvas; Owner: TDrawPoint);
begin
     Canvas.MoveTo(FromDO.x, FromDO.y);
     Canvas.LineTo(Owner.x, Owner.y);
end;
Procedure TSemanticNArrow.Draw(Canvas: TCanvas; Owner: TDrawPoint);
begin
     Canvas.MoveTo(FromDO.x, FromDO.y);
     Canvas.LineTo(Owner.x, Owner.y);
end;}
end.
