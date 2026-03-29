import { useState, useCallback, useRef } from "react";
import Editor from "./components/Editor";
import OutputPanel from "./components/OutputPanel";
import Toolbar from "./components/Toolbar";
import StatusBar from "./components/StatusBar";
import styles from "./App.module.css";

const API_BASE = import.meta.env.VITE_API_URL || "";

const DEFAULT_SOURCE = `program example;
var
  x, y : integer;
  result : integer;

begin
  read(x);
  read(y);
  result := x + y;
  write(result);
  if result > 10 then
  begin
    write(result)
  end
  else
  begin
    write(x)
  end
end.
`;

export default function App() {
  const [source, setSource] = useState(DEFAULT_SOURCE);
  const [output, setOutput] = useState(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);
  const [splitPos, setSplitPos] = useState(50); // percent
  const dragging = useRef(false);
  const containerRef = useRef(null);

  const compile = useCallback(async () => {
    setLoading(true);
    setError(null);
    try {
      const res = await fetch(`${API_BASE}/compile`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ source }),
      });
      if (!res.ok) {
        const e = await res.json();
        throw new Error(e.error || "Server error");
      }
      const data = await res.json();
      setOutput(data);
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  }, [source]);

  // ── Drag-to-resize ──────────────────────────────────────
  const onDividerMouseDown = useCallback((e) => {
    e.preventDefault();
    dragging.current = true;
    document.body.style.cursor = "col-resize";
    document.body.style.userSelect = "none";

    const onMove = (ev) => {
      if (!dragging.current || !containerRef.current) return;
      const rect = containerRef.current.getBoundingClientRect();
      const pct = ((ev.clientX - rect.left) / rect.width) * 100;
      setSplitPos(Math.min(75, Math.max(25, pct)));
    };
    const onUp = () => {
      dragging.current = false;
      document.body.style.cursor = "";
      document.body.style.userSelect = "";
      document.removeEventListener("mousemove", onMove);
      document.removeEventListener("mouseup", onUp);
    };
    document.addEventListener("mousemove", onMove);
    document.addEventListener("mouseup", onUp);
  }, []);

  const lineCount = source.split("\n").length;

  return (
    <div className={styles.root}>
      <Toolbar onCompile={compile} loading={loading} hasOutput={!!output} />

      <div className={styles.workspace} ref={containerRef}>
        {/* ── Left: Code Editor ─── */}
        <div className={styles.editorPane} style={{ width: `${splitPos}%` }}>
          <div className={styles.paneHeader}>
            <span className={styles.paneTitle}>Source</span>
            <span className={styles.paneHint}>Pascal</span>
          </div>
          <Editor value={source} onChange={setSource} />
        </div>

        {/* ── Divider ─── */}
        <div className={styles.divider} onMouseDown={onDividerMouseDown}>
          <div className={styles.dividerHandle} />
        </div>

        {/* ── Right: Output Tabs ─── */}
        <div className={styles.outputPane} style={{ width: `${100 - splitPos}%` }}>
          <OutputPanel
            output={output}
            loading={loading}
            error={error}
          />
        </div>
      </div>

      <StatusBar
        lines={lineCount}
        loading={loading}
        success={output?.success}
        errorCount={output?.errors?.length ?? null}
      />
    </div>
  );
}