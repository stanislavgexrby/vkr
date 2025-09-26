unit TransSemantic;

interface
uses
  classes, TransLeaf;

type
  TSemantic = class(TLeaf)
    function equals(aSemantic :TSemantic):boolean;
  end;

implementation

function TSemantic.equals(aSemantic :TSemantic):boolean;
begin
  result:=(getName = aSemantic.getName);
end;

end.

