unit ChangeSize;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, Mask;

type
  TChangeSizeForm = class(TForm)
    HeightEdit: TMaskEdit;
    WidthEdit: TMaskEdit;
    Button1: TButton;
    Button2: TButton;
    Label1: TLabel;
    Label2: TLabel;
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  ChangeSizeForm: TChangeSizeForm;

implementation

{$R *.DFM}

end.
