import { useState, useEffect } from "react";
import TokensTab from "./tabs/TokensTab";
import SymbolsTab from "./tabs/SymbolsTab";
import ErrorsTab from "./tabs/ErrorsTab";
import ParseTreeTab from "./tabs/ParseTreeTab";
import TACTab from "./tabs/TACTab";
import TargetTab from "./tabs/TargetTab";
import styles from "./OutputPanel.module.css";

const TABS = [
  { id: "tokens",    label: "Tokens" },
  { id: "symbols",   label: "Symbols" },
  { id: "errors",    label: "Errors" },
  { id: "tree",      label: "Parse Tree" },
  { id: "tac",       label: "TAC" },
  { id: "target",    label: "Target" },
];

export default function OutputPanel({ output, loading, error }) {
  const [active, setActive] = useState("tokens");

  // Auto-switch to errors tab if errors present
  useEffect(() => {
    if (output && output.errors?.length > 0) setActive("errors");
    else if (output && output.success)       setActive("tokens");
  }, [output]);

  const errCount = output?.errors?.length ?? 0;

  return (
    <div className={styles.panel}>
      {/* Tab bar */}
      <div className={styles.tabBar}>
        {TABS.map((tab) => (
          <button
            key={tab.id}
            className={`${styles.tab} ${active === tab.id ? styles.active : ""}`}
            onClick={() => setActive(tab.id)}
          >
            {tab.label}
            {tab.id === "errors" && errCount > 0 && (
              <span className={styles.errBadge}>{errCount}</span>
            )}
          </button>
        ))}
      </div>

      {/* Content */}
      <div className={styles.content}>
        {!output && !loading && !error && (
          <EmptyState />
        )}
        {loading && <LoadingState />}
        {error && !loading && (
          <div className={styles.fatalError}>
            <span className={styles.fatalIcon}>✗</span>
            <span>{error}</span>
          </div>
        )}
        {output && !loading && (
          <>
            {active === "tokens" && <TokensTab tokens={output.tokens} />}
            {active === "symbols" && <SymbolsTab symbols={output.symbols} />}
            {active === "errors" && <ErrorsTab errors={output.errors} />}
            {active === "tree" && <ParseTreeTab tree={output.tree} />}
            {active === "tac" && <TACTab tac={output.tac} success={output.success} />}
            {active === "target" && <TargetTab code={output.targetCode} success={output.success} />}
          </>
        )}
      </div>
    </div>
  );
}

function EmptyState() {
  return (
    <div className={styles.emptyState}>
      <div className={styles.emptyIcon}>
        <svg width="32" height="32" viewBox="0 0 32 32" fill="none">
          <rect x="4" y="4" width="24" height="24" rx="4" stroke="var(--border-light)" strokeWidth="1.5"/>
          <path d="M10 11h12M10 16h8M10 21h10" stroke="var(--border-light)" strokeWidth="1.5" strokeLinecap="round"/>
        </svg>
      </div>
      <p className={styles.emptyTitle}>No output yet</p>
      <p className={styles.emptyHint}>Press <kbd>Run</kbd> to compile your program</p>
    </div>
  );
}

function LoadingState() {
  return (
    <div className={styles.emptyState}>
      <div className={styles.loadDots}>
        <span /><span /><span />
      </div>
      <p className={styles.emptyHint}>Compiling…</p>
    </div>
  );
}