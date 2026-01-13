#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_CMD_LEN 1024 // אורך מקסימלי לפקודה
#define MAX_ARGS 64      // מקסימום ארגומנטים
#define DELIMITERS " \t\n" // מפרידים: רווח, טאב, ירידת שורה

int main() {
    char command_line[MAX_CMD_LEN];
    char *args[MAX_ARGS];
    char *token;
    int status;
    char path_buffer[MAX_CMD_LEN]; 

    // הלולאה הראשית של ה-Shell [cite: 17]
    while (1) {
        // 1. הצגת Prompt [cite: 19]
        printf("mini-bash$ ");
        fflush(stdout); // ניקוי באפר כדי להבטיח שההדפסה תופיע מיד

        // 2. קריאת פקודה [cite: 20]
        // שימוש ב-fgets לקריאה בטוחה לתוך הבאפר
        if (fgets(command_line, MAX_CMD_LEN, stdin) == NULL) {
            break; // יציאה מסודרת אם נקלט EOF (למשל Ctrl+D)
        }

        // 3. ניתוח (Parse) [cite: 21]
        // פירוק המחרוזת למילים באמצעות strtok (יעיל זיכרון, ללא הקצאות מיותרות) [cite: 49]
        int i = 0;
        token = strtok(command_line, DELIMITERS);
        while (token != NULL && i < MAX_ARGS - 1) {
            args[i++] = token;
            token = strtok(NULL, DELIMITERS);
        }
        args[i] = NULL; // סיום מערך הארגומנטים ב-NULL (חובה עבור execv)

        // אם לא הוכנסה פקודה (רק Enter), ממשיכים לסיבוב הבא
        if (args[0] == NULL) {
            continue;
        }

        // 4. זיהוי וביצוע - פקודות פנימיות [cite: 23, 24]

        // פקודת יציאה exit 
        if (strcmp(args[0], "exit") == 0) {
            break; 
        }

        // פקודת שינוי תיקייה cd 
        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                // אופציונלי: התנהגות כשאין ארגומנט (אפשר להתעלם או להעיר)
            } else {
                // ביצוע chdir [cite: 27]
                if (chdir(args[1]) != 0) {
                    perror("mini-bash: cd"); 
                }
            }
            continue; // פקודה פנימית בוצעה, חוזרים להתחלה
        }

        // 5. חיפוש פקודות חיצוניות [cite: 28]
        
        char *exec_path = NULL;
        int found = 0;

        // שלב א': חיפוש בתיקיית הבית (HOME) [cite: 30]
        char *home_dir = getenv("HOME"); // שליפת משתנה הסביבה HOME [cite: 31]
        if (home_dir != NULL) {
            snprintf(path_buffer, sizeof(path_buffer), "%s/%s", home_dir, args[0]);
            // בדיקה האם הקובץ קיים וניתן להרצה (X_OK) 
            if (access(path_buffer, X_OK) == 0) {
                exec_path = path_buffer;
                found = 1;
            }
        }

        // שלב ב': חיפוש בתיקיית /bin אם לא נמצא בבית [cite: 35]
        if (!found) {
            snprintf(path_buffer, sizeof(path_buffer), "/bin/%s", args[0]);
            if (access(path_buffer, X_OK) == 0) {
                exec_path = path_buffer;
                found = 1;
            }
        }

        // שלב ג': הודעת שגיאה אם לא נמצא בשום מקום [cite: 37]
        if (!found) {
            printf("[%s]: Unknown Command\n", args[0]); // הפורמט הנדרש [cite: 39]
            continue;
        }

        // 6. ניהול תהליכים (Process Management) [cite: 40]

        pid_t pid = fork(); // יצירת תהליך בן [cite: 42]

        if (pid < 0) {
            perror("fork failed"); // כישלון ביצירת תהליך
        } 
        else if (pid == 0) {
            // --- קוד תהליך הבן (Child Process) ---
            // החלפת התוכנית בזיכרון [cite: 43]
            // שימוש ב-execv שמקבל נתיב מלא (שמצאנו) ומערך ארגומנטים
            execv(exec_path, args);
            
            // אם הגענו לכאן, execv נכשל
            perror("execv failed"); // שימוש ב-perror [cite: 46]
            exit(EXIT_FAILURE);
        } 
        else {
            // --- קוד תהליך האב (Parent Process) ---
            // המתנה לסיום תהליך הבן 
            wait(&status);

            // דיווח על הצלחה וקוד חזרה [cite: 45]
            if (WIFEXITED(status)) {
                printf("Command executed successfully, Return Code: %d\n", WEXITSTATUS(status));
            }
        }
    }

    return 0;
}