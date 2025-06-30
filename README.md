# My CS50x Final Project: [Unreal fps movement system]

## Short Video Demo

**Watch my project in action!**

https://github.com/user-attachments/assets/95d9c40a-0e79-40c3-8d78-8c31d98ab764


## 💡 Project Overview

* **What is it?**
  * I made this project as a first person shooter movement system, inspired by titanfall 2's movement system.
* **Why did I build this?**
  * I made this project to create a fps movement system for a fps game that i plan to work on in the future. Inspired by titanfall 2's movement system. Making use of my previous experience in
game development with the unity game engine and the critical thinking skills i have learnt from CS50x.
* **Key Features:**
    * **Modular C++ Architecture:** Built for easy extension and customization of movement abilities.
    * **Dynamic Wall-Running:** Allows players to traverse vertical surfaces.
    * **Responsive Wall-Jumping:** Provides additional vertical and horizontal mobility off walls.
    * **Versatile Double Jumping:** Offers enhanced aerial control.
    * **Fluid Crouch-Sliding:** Enables rapid ground traversal and evasion.
    * **Integrated Sprinting System:** For efficient high-speed movement.
    * **Emphasis on Player Feel:** Tuned for intuitive and satisfying player control.

---

## 🛠️ Technologies Used
- **Engine**: Unreal Engine 5.5
- **Languages**: C++ (core logic), Blueprints (prototyping)
- **Development Tools**:
  - [Unreal Clangd](https://github.com/boocs/unreal-clangd) (VS Code Intellisense)
  - Visual Studio build tools 2022 
  - Unreal Engine Editor

---

## ⚙️ Installation

### Prerequisites
- Unreal Engine 5.5
- Visual Studio build tools 2022 with C++ Game Development tools
- Windows 10/11 (64-bit)

### Setup
1. **Clone repository**:
   ```bash
   git clone https://github.com/thatgreyCat7777/Unreal-Fps-Movement.git

2. **Open project:**

   * Launch Unreal Engine 5.5

   * Select Open Project → Navigate to cloned directory

3. **Build project:**

   * Right-click .uproject file → Generate Visual Studio project files

   * Open .sln in Visual Studio → Build solution


---

## 📂 Project Structure and Files

* Unreal-Fps-Movement/
* ├── Source/
* │   ├── FPSMovement/
* │   │   ├── FPSCharacter.h         # Character class interface
* │   │   ├── FPSCharacter.cpp       # Movement logic implementation
* │   │   ├── MovementComponents/    # Modular movement features
* │   │   │   ├── WallRunComponent.*
* │   │   │   ├── SlideComponent.*
* │   │   │   └── ...
* ├── Content/                       # UE assets (meshes, materials)
* ├── Config/                        # Project settings
* └── README.md                      # Project documentation

---

## 🧠 Design Choices and Challenges

* **Key Design Choices**
  * Modular C++ Architecture: I opted for a highly modular C++ design for the movement system. This wasn't just about good practice; it was crucial for managing complexity. By separating distinct movement abilities (like sprinting, sliding, and wall-running) into their own components or functions, I could develop and debug each independently. This approach also ensures the system is easily extensible, allowing for new movement mechanics to be added without major refactoring, and promotes reusability in other projects.

  * **Physics-Driven Movement:** Instead of purely animation-driven movement, I prioritized a physics-based approach where feasible. This decision aimed to make the player's interactions with the environment, especially during wall-runs and slides, feel more natural and responsive. For instance, slide momentum is influenced by the player's speed and surface friction, providing a tangible sense of inertia.

* **Challenges Faced and Solutions**
  * **Smooth Wall-Running Adhesion:** One significant challenge was ensuring consistent and smooth wall adhesion for wall-running, especially around corners or on slightly uneven surfaces. Initially, the character would sometimes detach if going too fast. My solution involved using the on hit function to gain access to wall normal info, coupled with calculated forces applied to keep the character "stuck" to the wall while still allowing forward momentum. This required careful tuning of force magnitudes and surface detection logic to feel natural.

---

## 🔮 Future Enhancements (Optional)

What are your plans for the project if you had more time? This shows forward-thinking and continued interest.

- [ ] Implement grapple hook mechanics
- [ ] Add momentum-based camera effects
- [ ] Network replication for multiplayer
- [ ] Implement movement into proper full game

---

## 🙏 Acknowledgements

* **CS50x:** "This project was completed as the final project for CS50x, Harvard University's introduction to the intellectual enterprises of computer science and the art of programming."
* **Resources:** Any specific tutorials, libraries, or individuals that greatly helped you.
    * Vs code unreal clangd extension (https://github.com/boocs/unreal-clangd) for helping me get intellisense working with unreal engine and vs code
    * "Special thanks to [pensivepanda5] for their valuable insights on project scope"

---

## ✍️ Author

* **[Lim Zhi Yuan Devon]**
* **GitHub:** [https://github.com/thatgreyCat7777](https://github.com/thatgreyCat7777)
* **CS50x edX Username:** [thatgreyCat7777]

