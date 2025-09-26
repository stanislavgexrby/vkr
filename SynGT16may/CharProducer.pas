unit CharProducer;

interface
const
  cEOF:char=#0;
type
    TCharProducer=class
    private
        m_curChar:char;
    public
        Function getChar:char;
        function next:boolean;virtual;abstract;
        property CurrentChar:char read getChar;
    end;
    TStringCharProducer=class(TCharProducer)
    private
        m_s:string;
        m_Length,m_index:integer;
    public
        constructor make(const s:string);
        function next:boolean;override;
    end;
    TFileCharProducer=class(TCharProducer)
    private
        m_F:TextFile;
    public
        constructor make(const FileName:string);
        function next:boolean;override;
        destructor destroy;
    end;
implementation

constructor TFileCharProducer.make(const FileName:string);
begin
  assign(m_F,FileName);
  reset(m_F);
  read(m_F,m_curChar);
end;

function TFileCharProducer.next:boolean;
begin
  if not EOF(m_F) then begin
    read(m_F,m_curChar);
    result:=true;
  end else begin
    m_curChar:=cEOF;
    result:=false;
  end;
end;

constructor TStringCharProducer.make(const s:string);
begin
  m_s:=s;
  m_Length:=Length(s);
  m_Index:=0;
  next;
end;
function TStringCharProducer.next:boolean;
begin
  if m_Index<m_Length then begin
    inc(m_Index);
    m_curChar:=m_s[m_Index];
    result:=true;
  end else begin
    m_curChar:=cEOF;
    result:=false;
  end;
end;
Function TCharProducer.getChar:char;
begin
  result:=m_curChar;
end;
destructor TFileCharProducer.destroy;
begin
  system.close(m_F);
  inherited destroy;
end;

end.
