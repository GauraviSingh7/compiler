import styles from "./Tab.module.css";

export default function ErrorsTab({ errors }) {
  if (!errors?.length)
    return <div className={styles.ok}>✓ No errors detected.</div>;

  return (
    <div className={styles.errorList}>
      {errors.map((e, i) => {
        // Try to pull out "Line N" prefix for highlighting
        const match = e.match(/^Line (\d+):\s*(.*)/);
        return (
          <div key={i} className={styles.errorItem}>
            <span className={styles.errorIcon}>✗</span>
            <div className={styles.errorBody}>
              {match ? (
                <>
                  <span className={styles.errorLine}>Line {match[1]}</span>
                  <span className={styles.mono}>{match[2]}</span>
                </>
              ) : (
                <span className={styles.mono}>{e}</span>
              )}
            </div>
          </div>
        );
      })}
    </div>
  );
}