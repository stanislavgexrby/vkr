unit PrintForm;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  ExtCtrls;

type
  TBitmapPrintForm = class(TForm)
    procedure FormPaint(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
    offScreen:TBitmap;
    procedure SetOffScreen(AOffScreen:TBitmap);
  end;

var
  BitmapPrintForm: TBitmapPrintForm;

implementation

{$R *.DFM}
procedure TBitmapPrintForm.SetOffScreen(AOffScreen:TBitmap);
begin
  offScreen:=AOffScreen;
end;

procedure TBitmapPrintForm.FormPaint(Sender: TObject);
begin
  Canvas.Draw(0,0,offScreen);
end;

end.
