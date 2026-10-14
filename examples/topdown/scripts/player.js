// PenguinEngine: script workspace example.
// Syntax checking only; gameplay bindings will follow later.

const speed = 90;

export function update(player, input, delta) {
    const direction = {
        x: input.right - input.left,
        y: input.down - input.up,
    };

    const length = Math.max(1, Math.sqrt(
        direction.x * direction.x + direction.y * direction.y
    ));

    return {
        x: player.x + direction.x / length * speed * delta,
        y: player.y + direction.y / length * speed * delta,
        message: "Hello, Penguin!",
    };
}
