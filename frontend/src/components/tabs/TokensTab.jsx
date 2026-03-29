import styles from "./Tab.module.css";

export default function TokensTab({ tokens }) {
  if (!tokens?.length)
    return <div className={styles.empty}>No tokens.</div>;

  return (
    <table className={styles.table}>
      <thead>
        <tr>
          <th>Line</th>
          <th>Type</th>
          <th>Lexeme</th>
        </tr>
      </thead>
      <tbody>
        {tokens.map((t, i) => (
          <tr key={i}>
            <td className={styles.num}>{t.line}</td>
            <td className={styles.type}>{t.type}</td>
            <td className={styles.mono}>{t.lexeme}</td>
          </tr>
        ))}
      </tbody>
    </table>
  );
}