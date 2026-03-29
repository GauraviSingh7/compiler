import styles from "./Toolbar.module.css";

export default function Toolbar({ onCompile, loading, hasOutput }) {
  return (
    <header className={styles.toolbar}>
      <div className={styles.left}>
        <div className={styles.logo}>
          <svg width="18" height="18" viewBox="0 0 18 18" fill="none">
            <rect x="1" y="1" width="16" height="16" rx="3" stroke="var(--accent)" strokeWidth="1.5"/>
            <path d="M5 6l3 3-3 3" stroke="var(--accent)" strokeWidth="1.5" strokeLinecap="round" strokeLinejoin="round"/>
            <path d="M10 12h3" stroke="var(--accent)" strokeWidth="1.5" strokeLinecap="round"/>
          </svg>
        </div>
        <span className={styles.title}>Pascal Compiler</span>
        <span className={styles.badge}>IDE</span>
      </div>

      <div className={styles.right}>
        <button
          className={styles.compileBtn}
          onClick={onCompile}
          disabled={loading}
        >
          {loading ? (
            <>
              <span className={styles.spinner} />
              Compiling…
            </>
          ) : (
            <>
              <svg width="13" height="13" viewBox="0 0 13 13" fill="none">
                <polygon points="2,1 12,6.5 2,12" fill="currentColor"/>
              </svg>
              Run
            </>
          )}
        </button>
      </div>
    </header>
  );
}