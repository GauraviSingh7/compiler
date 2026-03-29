import styles from "./Tab.module.css";

export default function SymbolsTab({ symbols }) {
  if (!symbols?.length)
    return <div className={styles.empty}>No symbols declared.</div>;

  return (
    <table className={styles.table}>
      <thead>
        <tr>
          <th>Name</th>
          <th>Kind</th>
          <th>Array Size</th>
        </tr>
      </thead>
      <tbody>
        {symbols.map((s, i) => (
          <tr key={i}>
            <td className={styles.mono}>{s.name}</td>
            <td>
              <span
                className={`${styles.kindBadge} ${
                  styles[s.kind.toLowerCase()]
                }`}
              >
                {s.kind}
              </span>
            </td>
            <td className={styles.num}>
              {s.kind === "ARRAY" ? s.arraySize : "—"}
            </td>
          </tr>
        ))}
      </tbody>
    </table>
  );
}