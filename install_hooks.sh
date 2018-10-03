#!/usr/bin/env bash

echo "#!/usr/bin/env bash" > .git/hooks/pre-commit
echo "$(git rev-parse --show-toplevel)/scripts/build.py -t" >> .git/hooks/pre-commit
chmod u+x .git/hooks/pre-commit

